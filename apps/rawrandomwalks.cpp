#define DYNAMICEDATA 1
#include <string>
#include <fstream>
#include <vector>
#include <cmath>

#include "api/graphwalker_basic_includes.hpp"
#include "walks/randomwalkwithjump.hpp"

class RawRandomWalks : public RandomWalkwithJump{

public:

    // tid_t exec_threads;
    // eid_t *used_edges;

    void initializeApp(vid_t N, wid_t R, hid_t L){
        initializeRW(N,R,L);
        // exec_threads = get_option_int("execthreads", omp_get_max_threads());
        // used_edges = new eid_t[exec_threads];
        // for(int i=0; i<exec_threads; i++){
        //     used_edges[i] = 0;
        // }
        // unlink("graphwalker_utilization.csv"); 
    }

    void startWalksbyApp(WalkManager &walk_manager){
        std::cout << "Random walks:\tStart " << R << " walks randomly ..." << std::endl;
        srand((unsigned)time(NULL));
        tid_t exec_threads = get_option_int("execthreads", omp_get_max_threads());
        omp_set_num_threads(exec_threads);
        #pragma omp parallel for schedule(static)
            for (wid_t i = 0; i < R; i++){
                vid_t s = rand()%N;
                bid_t p = getblock(s);
                vid_t cur = s - blocks[p];
                WalkDataType walk = walk_manager.encode(s,cur,0);
                walk_manager.moveWalk(walk,p,omp_get_thread_num(),cur);
            }
        for( bid_t p = 0; p < nblocks; p++){
            walk_manager.walknum[p] = walk_manager.dwalknum[p];
            for(tid_t t = 0; t < exec_threads; t++)
                walk_manager.walknum[p] +=  walk_manager.pwalks[t][p].size_w;
            if(walk_manager.walknum[p] )
                walk_manager.minstep[p] = 0;
        }
        walk_manager.walksum = R;
    }

    void updateInfo(vid_t s, vid_t dstId, tid_t threadid, hid_t hop){
        // used_edges[threadid]++;
    }

    
    void compUtilization(eid_t total_edges){
        // for(tid_t i = 1; i < exec_threads; i++){
        //     used_edges[0] += used_edges[i];
        // }

        // float utilization = (float)used_edges[0] / (float)total_edges;
        // std::string utilization_filename = "graphwalker_utilization.csv";
        // std::ofstream utilizationfile;
        // utilizationfile.open(utilization_filename.c_str(), std::ofstream::app);
        // utilizationfile << total_edges << "\t" << used_edges[0] << "\t" << utilization << "\n" ;
        // utilizationfile.close();

        // for(tid_t i=0; i<exec_threads; i++){
        //     used_edges[i] = 0;
        // }
    }

};

int main(int argc, const char ** argv){
    set_argc(argc,argv);
    metrics m("randomwalks");
    
    std::string filename = get_option_string("file", "../../raid0_mnop/LiveJournal/soc-LiveJournal1.txt");  // Base filename
    vid_t N = get_option_int("N", 4847571); // Number of vertices
    wid_t R = get_option_long("R", 10000); // Number of steps
    hid_t L = get_option_int("L", 4); // Number of steps per walk
    float prob = get_option_float("prob", 0.2); // prob of chose min step
    unsigned long long blocksize_kb = get_option_long("blocksize_kb", 0); // Size of block, represented in KB
    bid_t nmblocks = get_option_int("nmblocks", 0); // number of in-memory blocks
    
    logstream(LOG_DEBUG) << "N R L : " << N << " " << R << " " << L << std::endl;
    // run
    RawRandomWalks program;
    program.initializeApp(N,R,L);

    /* Detect the number of shards or preprocess an input to create them */
    if(blocksize_kb == 0){
        blocksize_kb = program.compBlockSize(R);
    }
    if(nmblocks == 0){
        nmblocks = program.compNmblocks(blocksize_kb);
    }
    bid_t nblocks = convert_if_notexists(filename, blocksize_kb);
    if(nmblocks > nblocks) nmblocks = nblocks;

    logstream(LOG_DEBUG) << "nblocks nmblocks : " << nblocks << " " << nmblocks << std::endl;

    graphwalker_engine engine(filename, blocksize_kb, nblocks,nmblocks, m);
    engine.run(program, prob);

    metrics_report(m);
    return 0;
}