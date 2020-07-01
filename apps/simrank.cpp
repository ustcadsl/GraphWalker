#include <string>
#include <fstream>
#include <vector>
#include <cmath>

#include "api/graphwalker_basic_includes.hpp"
#include "walks/randomwalkwithrestartwithjoint.hpp"

class SimRank : public RandomWalkwithRestartwithJoint{
private:
	vid_t a, b;
	std::vector<vid_t> walksfroma; //record the path of walks
	std::vector<vid_t> walksfromb;
    std::string basefilename;
    vid_t cur_window_st;

public:
	void initializeApp( vid_t _a, vid_t _b, wid_t _R, hid_t _L ){
		a = _a;
		b = _b;
		R = _R;
		L = _L;
		walksfroma.resize(R*L);
		walksfromb.resize(R*L);
		memset(walksfroma.data(), 0xff, walksfroma.size()*sizeof(vid_t));
		memset(walksfromb.data(), 0xff, walksfromb.size()*sizeof(vid_t));
		initializeRW( R, L);
	}

	void startWalksbyApp(WalkManager &walk_manager){
		// R random walk start from a
		logstream(LOG_INFO) << "Start " << R << " walks of length " << L << " from a and b : " << std::endl;
		tid_t nthreads = get_option_int("execthreads", omp_get_max_threads());
		bid_t p = getblock(a);
		assert( p == getblock(b) );
		vid_t cur = a - blocks[p];
		walk_manager.minstep[p] = 0;
		walk_manager.walknum[p] = 2*R;

		//start R walks from a
		omp_set_num_threads(nthreads);
		#pragma omp parallel for schedule(static)
			for(wid_t i = 0; i < R; i++){
				hid_t hop = i * L;
				WalkDataType walk = walk_manager.encode(a, cur, hop);
				walk_manager.moveWalk(walk,p,omp_get_thread_num(),cur);
			}
		//start R walks from b
		cur = b - blocks[p];
		#pragma omp parallel for schedule(static)
			for(wid_t i = 0; i < R; i++){
				hid_t hop = i * L;
				WalkDataType walk = walk_manager.encode(b, cur, hop);
				walk_manager.moveWalk(walk,p,omp_get_thread_num(),cur);
			}
		walk_manager.walksum = 2*R;
    }

    void updateInfo(vid_t s, vid_t dstId, tid_t threadid, hid_t hop){
		if( s == a )
			walksfroma[hop] = dstId;
		else if( s == b )
			walksfromb[hop] = dstId;
		else{
			logstream(LOG_ERROR) << "Wrong source s : " << s << std::endl;
			assert(0);
		}
	}

	float computeResult(){
		float simrank = 0;
		for( wid_t i = 0; i < R; i++ )
			for( wid_t j = 0; j < R; j++ )
				for( wid_t l = 0; l < L; l++ ){
					if( walksfroma[i*L+l] == walksfromb[j*L+l] && walksfroma[i*L+l] != 0xffffffff ){
						simrank += (1.0/(R*R))*pow(0.8, l);
						break;
					}
				}
		return simrank;
	}

};

int main(int argc, const char ** argv) {
    /* Read the command line arguments and the configuration file. */
    set_argc(argc, argv);  
    /* Metrics object for keeping track of performance count_invectorers and other information. Currently required. */
    metrics m("simrank");
    
    /* Basic arguments for application */
    std::string filename = get_option_string("file", "../DataSet/LiveJournal/soc-LiveJournal1.txt");  // Base filename
    vid_t a = get_option_int("a", 1); // vertex id of start source
    vid_t b = get_option_int("b", 2); // Number of sources
    wid_t R = get_option_int("R", 1000); // Number of steps
    hid_t L = get_option_int("L", 11); // Number of steps per walk
    float prob = get_option_float("prob", 0.2); // prob of chose min step
    unsigned long long blocksize_kb = get_option_long("blocksize_kb", 0); // Size of block, represented in KB
    bid_t nmblocks = get_option_int("nmblocks", 0); // number of in-memory blocks
    
    /* Run */
    SimRank program;
    program.initializeApp( a, b, R, L );
    
	if(blocksize_kb == 0)
        blocksize_kb = program.compBlockSize(2*R);
    /* Detect the number of shards or preprocess an input to create them */
    bid_t nblocks = convert_if_notexists(filename, blocksize_kb);
    if(nmblocks == 0) nmblocks = program.compNmblocks(blocksize_kb);
    if(nmblocks > nblocks) nmblocks = nblocks;

    graphwalker_engine engine(filename, blocksize_kb, nblocks,nmblocks, m);
    engine.run(program, prob);

    float simrank = program.computeResult();
    std::cout << "SimRank for " << a << " and " << b << " = " << simrank << std::endl;
    
    /* Report execution metrics */
    metrics_report(m);
    return 0;
}