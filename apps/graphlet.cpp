#include <string>
#include <fstream>
#include <vector>
#include <cmath>

#include "api/graphwalker_basic_includes.hpp"
#include "walks/randomwalkwithstop.hpp"

class graphLet : public RandomWalkwithStop{
    private:
        vid_t N;
        wid_t *cnt_ok;

    public:
        void initializeApp(vid_t _N, wid_t _R, hid_t _L){
            N = _N;
            R = _R;
            L = _L;
            cnt_ok = 0;
            initializeRW(R, L);
        }

        void startWalksbyApp(WalkManager &walk_manager){
            srand((unsigned)time(NULL));
            tid_t nthreads = get_option_int("execthreads", omp_get_max_threads());
            omp_set_num_threads(nthreads);
            // #pragma omp parallel for schedule(static)
                for (wid_t i = 0; i < R; i++){
                    vid_t s = rand()%N;
                    bid_t p = getblock(s);
                    vid_t cur = s - blocks[p];
                    WalkDataType walk = walk_manager.encode(s,cur,0);
                    walk_manager.moveWalk(walk,p,omp_get_thread_num(),cur);
                    walk_manager.minstep[p] = 0;
                    walk_manager.walknum[p]++;
                }
            cnt_ok = new wid_t [nthreads];
            for(tid_t i = 0; i < nthreads; i++ ){
                cnt_ok[i] = 0;
            }
            walk_manager.walksum = R;
        }

		void updateInfo(vid_t s, vid_t dstId, tid_t threadid, hid_t hop){
            if(hop < L-1) return;
            if (dstId == s){
                cnt_ok[threadid]++;
            }
        }

        float computeResult(){
            tid_t nthreads = get_option_int("execthreads", omp_get_max_threads());
            for(tid_t i = 1; i < nthreads; i++ ){
                cnt_ok[0] += cnt_ok[i];
            }
            float triangle_ratio = (float) cnt_ok[0] / (float) R;
            return triangle_ratio;
        }

        wid_t getCntOK(){
            return cnt_ok[0];
        }
};

int main(int argc, const char ** argv){
    set_argc(argc,argv);
    metrics m("graphlet");
    
    std::string filename = get_option_string("file", "../dataset/LiveJournal/soc-LiveJournal1.txt");  // Base filename
    unsigned N = get_option_int("N", 4847571); // Number of vertices
    unsigned R = get_option_int("R", 100000); // Number of steps
    unsigned L = get_option_int("L", 4); // Number of steps per walk
    float prob = get_option_float("prob", 0.2); // prob of chose min step
    unsigned long long blocksize_kb = get_option_long("blocksize_kb", 0); // Size of block, represented in KB
    bid_t nmblocks = get_option_int("nmblocks", 0); // number of in-memory blocks
    
    // run
    graphLet program;
    program.initializeApp(N,R,L);

    if(blocksize_kb == 0)
        blocksize_kb = program.compBlockSize(R);
    /* Detect the number of shards or preprocess an input to create them */
    bid_t nblocks = convert_if_notexists(filename, blocksize_kb);
    if(nmblocks == 0) nmblocks = program.compNmblocks(blocksize_kb);
    if(nmblocks > nblocks) nmblocks = nblocks;

    graphwalker_engine engine(filename, blocksize_kb,nblocks,nmblocks, m);
    engine.run(program, prob);

    float triangle_ratio = program.computeResult();
    std::cout << "The ratio of triangle:\t" << triangle_ratio << std::endl;
    std::cout << "cnt_all:\t" << R << std::endl;
    std::cout << "cnt_ok:\t" << program.getCntOK() << std::endl;

    metrics_report(m);
    return 0;
}