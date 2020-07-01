#ifndef RANDOMWALK
#define RANDOMWALK

#include <string>
#include <fstream>
#include <time.h>

#include "walks/walk.hpp" 
#include "api/datatype.hpp"

/**
 * Type definitions. Remember to create suitable graph shards using the
 * Sharder-program.
 */
 
class RandomWalk {

public:
    bid_t nblocks;
    vid_t *blocks;
    wid_t R;
    hid_t L;

public:

    //for SimRank
    virtual void startWalksbyApp( WalkManager &walk_manager){
        logstream(LOG_ERROR) << "No definition of function : startWalksbyApp!" << std::endl;
    }  

    //for all
    virtual void updateInfo(vid_t s, vid_t dstId, tid_t threadid, hid_t hop){
        logstream(LOG_ERROR) << "No definition of function : updateInfo!" << std::endl;
    }

    /**
     *  Walk update function.
     */
    virtual void updateByWalk(WalkDataType walk, wid_t walkid, bid_t exec_block, eid_t *&beg_pos, vid_t *&csr, WalkManager &walk_manager ){ //, VertexDataType* vertex_value){
        logstream(LOG_ERROR) << "No definition of function : updateByWalk!" << std::endl;
    }
    
    /**
     * Called before an execution block is started.
     */
    virtual wid_t before_exec_block(bid_t exec_block, vid_t window_st, vid_t window_en, WalkManager &walk_manager) {
        logstream(LOG_DEBUG) << "No definition of function : before_exec_block!" << std::endl;
        return 0;
    }
    
    /**
     * Called after an execution block has finished.
     */
    // virtual void after_exec_block(unsigned exec_block, vid_t window_st, vid_t window_en, WalkManager &walk_manager, Vertex *&vertices) {
    virtual void after_exec_block(bid_t exec_block, vid_t window_st, vid_t window_en, WalkManager &walk_manager) {
        logstream(LOG_DEBUG) << "No definition of function : after_exec_block!" << std::endl;
    }

    virtual void compUtilization(eid_t total_edges){
        logstream(LOG_DEBUG) << "No definition of function : compUtilization!" << std::endl;
    }

    virtual void initializeRW( wid_t _R, hid_t _L) {
        R = _R;
        L = _L;
    }

    virtual void startWalks(WalkManager &walk_manager, bid_t _nblocks, vid_t* _blocks, std::string base_filename){
        nblocks = _nblocks;
        blocks = _blocks;
        startWalksbyApp(walk_manager);
    }

    virtual unsigned getblock( vid_t v ){
        for( unsigned p = 0; p < nblocks; p++ ){
            if( v < blocks[p+1] )
                return p;
        }
        // assert(false);
        return nblocks;
    }
    
    /**
     * check if it has finished all walks
     */
    virtual bool hasFinishedWalk(WalkManager &walk_manager){
        wid_t remaining_walknum = walk_manager.walksum;
        return ( remaining_walknum > 0 ); 
    }

    /**
     * determine the block size according to the number of walks
     */
    virtual unsigned long long compBlockSize(wid_t nwalks){
        int dom = 0;
        while(nwalks){
            dom++;
            nwalks /= 10;
        }
        unsigned long long blocksize_kb = pow(2, 11 + dom);
        logstream(LOG_DEBUG) << "Determined blocksize_kb = " << blocksize_kb << std::endl;
        return blocksize_kb; 
    }

    /**
     * determine the number of in-memory blocks
     */
    virtual bid_t compNmblocks(unsigned long long blocksize_kb){
        bid_t nmblocks = MEM_BUDGET / blocksize_kb;
        logstream(LOG_DEBUG) << "Computed nmblocks = " << nmblocks << std::endl;
        return nmblocks; 
    }
    
};

#endif