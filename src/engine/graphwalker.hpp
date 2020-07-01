
#ifndef DEF_GRAPHCHWALKER_ENGINE
#define DEF_GRAPHCHWALKER_ENGINE

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <omp.h>
#include <vector>
#include <map>
#include <sys/time.h>
#include <sys/mman.h>
#include <asm/mman.h>

#include "api/filename.hpp"
#include "api/io.hpp"
#include "logger/logger.hpp"
#include "metrics/metrics.hpp"
#include "api/pthread_tools.hpp"
#include "walks/randomwalk.hpp"

class graphwalker_engine {
public:     
    std::string base_filename;
    // unsigned membudget_mb;
    unsigned long long blocksize_kb;  
    bid_t nblocks;  
    vid_t nvertices;      
    tid_t exec_threads;
    vid_t* blocks;
    timeval start;
    
    /* Ｉn memory blocks */
    bid_t nmblocks; //number of in memory blocks
    vid_t **csrbuf;
    eid_t **beg_posbuf;
    bid_t cmblocks; //current number of in memory blocks
    bid_t *inMemIndex;
    int beg_posf, csrf;

    /* State */
    bid_t exec_block;
    
    /* Metrics */
    metrics &m;
    WalkManager *walk_manager;
        
    void print_config() {
        logstream(LOG_INFO) << "Engine configuration: " << std::endl;
        logstream(LOG_INFO) << " exec_threads = " << (int)exec_threads << std::endl;
        logstream(LOG_INFO) << " blocksize_kb = " << blocksize_kb << "kb" << std::endl;
        logstream(LOG_INFO) << " number of total blocks = " << nblocks << std::endl;
        logstream(LOG_INFO) << " number of in-memory blocks = " << nmblocks << std::endl;
    }

    double runtime() {
            timeval end;
            gettimeofday(&end, NULL);
            return end.tv_sec-start.tv_sec+ ((double)(end.tv_usec-start.tv_usec))/1.0E6;
        }
        
public:
        
    /**
     * Initialize GraphChi engine
     * @param base_filename prefix of the graph files
     * @param nblocks number of shards
     * @param selective_scheduling if true, uses selective scheduling 
     */
    graphwalker_engine(std::string _base_filename, unsigned long long _blocksize_kb, bid_t _nblocks, bid_t _nmblocks, metrics &_m) : base_filename(_base_filename), blocksize_kb(_blocksize_kb), nblocks(_nblocks), nmblocks(_nmblocks), m(_m) {
        // membudget_mb = get_option_int("membudget_mb", 1024);
        exec_threads = get_option_int("execthreads", omp_get_max_threads());
        omp_set_num_threads(exec_threads);
        load_block_range(base_filename, blocksize_kb, blocks);
        logstream(LOG_INFO) << "block_range loaded!" << std::endl;
        nvertices = num_vertices();
        walk_manager = new WalkManager(m,nblocks,exec_threads,base_filename);
        logstream(LOG_INFO) << "walk_manager created!" << std::endl;

        csrbuf = (vid_t**)malloc(nmblocks*sizeof(vid_t*));
        for(bid_t b = 0; b < nmblocks; b++){
            csrbuf[b] = (vid_t*)malloc(blocksize_kb*1024);
            // csrbuf[b] = (vid_t *)mmap(NULL, blocksize_kb*1024,
            //         PROT_READ | PROT_WRITE,MAP_PRIVATE | MAP_ANONYMOUS 
            //         //| MAP_HUGETLB | MAP_HUGE_2MB
            //         , 0, 0);
            // if(csrbuf[b] == MAP_FAILED){
            //     printf("%lld\n",b*blocksize_kb*1024);
            //     perror("csrbuf alloc mmap");
            //     exit(-1);
            // }
        }
        logstream(LOG_INFO) << "csrbuf malloced!" << std::endl;
        beg_posbuf = (eid_t**)malloc(nmblocks*sizeof(eid_t*));
        inMemIndex = (bid_t*)malloc(nblocks*sizeof(bid_t));
        for(bid_t b = 0; b < nblocks; b++)  inMemIndex[b] = nmblocks;
        cmblocks = 0;

        // m.start_time("__g_loadSubGraph_filename");
        std::string invlname = fidname( base_filename, 0 ); //only 1 file
        std::string beg_posname = invlname + ".beg_pos";
        std::string csrname = invlname + ".csr";
        // m.stop_time("__g_loadSubGraph_filename");
        // m.start_time("__g_loadSubGraph_open_begpos");
        beg_posf = open(beg_posname.c_str(),O_RDONLY | O_CREAT, S_IROTH | S_IWOTH | S_IWUSR | S_IRUSR);
        // m.stop_time("__g_loadSubGraph_open_begpos");
        // m.start_time("__g_loadSubGraph_open_csr");
        csrf = open(csrname.c_str(),O_RDONLY | O_CREAT, S_IROTH | S_IWOTH | S_IWUSR | S_IRUSR);
        // m.stop_time("__g_loadSubGraph_open_csr");
        // m.start_time("__g_loadSubGraph_if_open_success");
        if (csrf < 0 || beg_posf < 0) {
            logstream(LOG_FATAL) << "Could not load :" << csrname << " or " << beg_posname << ", error: " << strerror(errno) << std::endl;
        }
        assert(csrf > 0 && beg_posf > 0);
        // m.stop_time("__g_loadSubGraph_if_open_success");

        _m.set("file", _base_filename);
        _m.set("engine", "default");
        _m.set("nblocks", (size_t)nblocks);

        print_config();
    }
        
    virtual ~graphwalker_engine() {
        delete walk_manager;
        
        if(inMemIndex != NULL) free(inMemIndex);
        if(blocks != NULL) free(blocks);

        for(bid_t b = 0; b < cmblocks; b++){
            if(beg_posbuf[b] != NULL)   free(beg_posbuf[b]);
            if(csrbuf[b] != NULL)   free(csrbuf[b]);
                // munmap(csrbuf[b], blocksize_kb*1024);
        }
        if(beg_posbuf != NULL) free(beg_posbuf);
        if(csrbuf != NULL) free(csrbuf);

        close(beg_posf);  
        close(csrf);  
    }

    void load_block_range(std::string base_filename, unsigned long long blocksize_kb, vid_t * &blocks, bool allowfail=false) {
        std::string blockrangefile = blockrangename(base_filename, blocksize_kb);
        std::ifstream brf(blockrangefile.c_str());
        
        if (!brf.good()) {
            logstream(LOG_ERROR) << "Could not load block range file: " << blockrangefile << std::endl;
        }
        assert(brf.good());
        
        blocks = (vid_t*)malloc((nblocks+1)*sizeof(vid_t));
        vid_t en;
        for(bid_t i=0; i < nblocks+1; i++) {
            assert(!brf.eof());
            brf >> en;
            blocks[i] = en;
        }
        for(bid_t i=nblocks-1; i < nblocks; i++) {
             logstream(LOG_INFO) << "last shard: " << blocks[i] << " - " << blocks[i+1] << std::endl;
        }
        brf.close();
    }

    void loadSubGraph(bid_t p, eid_t * &beg_pos, vid_t * &csr, vid_t *nverts, eid_t *nedges){
        m.start_time("g_loadSubGraph");
        

        // m.start_time("__g_loadSubGraph_malloc_begpos");
        /* read beg_pos file */
        *nverts = blocks[p+1] - blocks[p];
        beg_pos = (eid_t*) malloc((*nverts+1)*sizeof(eid_t));
        // m.stop_time("__g_loadSubGraph_malloc_begpos");
        // beg_pos=(eid_t *)mmap(NULL,(size_t)(*nverts+1)*sizeof(eid_t),
        //         PROT_READ | PROT_WRITE,MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
        // if(beg_pos == MAP_FAILED)
        // {
        //     printf("%ld\n",(size_t)(*nverts+1)*sizeof(eid_t));
        //     perror("beg_pos alloc mmap");
        //     exit(-1);
        // }
        m.start_time("z__g_loadSubGraph_read_begpos");
        preada(beg_posf, beg_pos, (size_t)(*nverts+1)*sizeof(eid_t), (size_t)blocks[p]*sizeof(eid_t));        
        m.stop_time("z__g_loadSubGraph_read_begpos");
        /* read csr file */
        m.start_time("z__g_loadSubGraph_realloc_csr");
        *nedges = beg_pos[*nverts] - beg_pos[0];
        if(*nedges*sizeof(vid_t) > blocksize_kb*1024){
            csr = (vid_t*)realloc(csr, (*nedges)*sizeof(vid_t) );
        }
        m.stop_time("z__g_loadSubGraph_realloc_csr");     
        m.start_time("z__g_loadSubGraph_read_csr");
        preada(csrf, csr, (*nedges)*sizeof(vid_t), beg_pos[0]*sizeof(vid_t));
        m.stop_time("z__g_loadSubGraph_read_csr");     

        m.stop_time("g_loadSubGraph");
    }

    void findSubGraph(bid_t p, eid_t * &beg_pos, vid_t * &csr, vid_t *nverts, eid_t *nedges){
        m.start_time("2_findSubGraph");
        if(inMemIndex[p] == nmblocks){//the block is not in memory
            bid_t swapin;
            if(cmblocks < nmblocks){
                swapin = cmblocks++;
            }else{
                bid_t minmwb = swapOut();
                swapin = inMemIndex[minmwb];
                inMemIndex[minmwb] = nmblocks;
                assert(swapin < nmblocks);
                if(beg_posbuf[swapin] != NULL) free(beg_posbuf[swapin]);
                    // munmap(beg_posbuf[swapin], sizeof(eid_t)*(blocks[minmwb+1] - blocks[minmwb] + 1));
            }
            loadSubGraph(p, beg_posbuf[swapin], csrbuf[swapin], nverts, nedges);
            inMemIndex[p] = swapin;
        }else{
        }
        beg_pos = beg_posbuf[ inMemIndex[p] ];
        csr = csrbuf[ inMemIndex[p] ];
        m.stop_time("2_findSubGraph");
    }

    bid_t swapOut(){
        m.start_time("z_g_swapOut");
        wid_t minmw = 0xffffffff;
        bid_t minmwb = 0;
        for(bid_t b = 0; b < nblocks; b++){
            if(inMemIndex[b]<nmblocks && walk_manager->walknum[b] < minmw){
                minmw = walk_manager->walknum[b];
                minmwb = b;
            }
        }
        m.start_time("z_g_swapOut");
        return minmwb;
    }

    virtual size_t num_vertices() {
        return blocks[nblocks];
    }

    void exec_updates(RandomWalk &userprogram, wid_t nwalks, eid_t *&beg_pos, vid_t *&csr){ //, VertexDataType* vertex_value){
        // unsigned count = walk_manager->readblockWalks(exec_block);
        m.start_time("5_exec_updates");
        if(nwalks < 100) omp_set_num_threads(1);
        #pragma omp parallel for schedule(static)
            for(wid_t i = 0; i < nwalks; i++ ){
                WalkDataType walk = walk_manager->curwalks[i];
                userprogram.updateByWalk(walk, i, exec_block, beg_pos, csr, *walk_manager );//, vertex_value);
            }
        m.stop_time("5_exec_updates");
        // walk_manager->writeblockWalks(exec_block);
    }

    void run(RandomWalk &userprogram, float prob) {
        // srand((unsigned)time(NULL));
        m.start_time("0_startWalks");
        userprogram.startWalks(*walk_manager, nblocks, blocks, base_filename);
        m.stop_time("0_startWalks");
        
        gettimeofday(&start, NULL);
        m.start_time("00_runtime");

        vid_t nverts, *csr;
        eid_t nedges, *beg_pos;
        /*loadOnDemand -- block loop */
        int blockcount = 0;
        while( userprogram.hasFinishedWalk(*walk_manager) ){
            blockcount++;
            m.start_time("1_chooseBlock");
            exec_block = walk_manager->chooseBlock(prob);
            m.stop_time("1_chooseBlock");
            findSubGraph(exec_block, beg_pos, csr, &nverts, &nedges);

            /*load walks info*/
            // walk_manager->loadWalkPool(exec_block);
            wid_t nwalks; 
            nwalks = walk_manager->getCurrentWalks(exec_block);
            
            // if(blockcount % (nblocks/100+1)==1)
            if(blockcount % (1024*1024*1024/nedges+1) == 1)
            {
                logstream(LOG_DEBUG) << runtime() << "s : blockcount: " << blockcount << std::endl;
                logstream(LOG_INFO) << "nverts = " << nverts << ", nedges = " << nedges << std::endl;
                logstream(LOG_INFO) << "walksum = " << walk_manager->walksum << ", nwalks[" << exec_block << "] = " << nwalks << std::endl;
            }
            
            exec_updates(userprogram, nwalks, beg_pos, csr);
            walk_manager->updateWalkNum(exec_block);
            // userprogram.compUtilization(beg_pos[nverts] - beg_pos[0]);

        } // For block loop
        m.stop_time("00_runtime");
    }
};

#endif