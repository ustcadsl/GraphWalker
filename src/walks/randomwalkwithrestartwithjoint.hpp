#ifndef RANDOMWALKWITHRESTARTWITHJOINT
#define RANDOMWALKWITHRESTARTWITHJOINT

#include <string>
#include <fstream>
#include <time.h>

#include "walks/walk.hpp" 
#include "api/datatype.hpp"

/**
 * Type definitions. Remember to create suitable graph shards using the
 * Sharder-program.
 */
 
class RandomWalkwithRestartwithJoint : public RandomWalk {

public:
    wid_t R;
    hid_t L;

public:

    void initializeRW(wid_t _R, hid_t _L){
        R = _R;
        L = _L;
    }

    void updateByWalk(WalkDataType walk, wid_t walkid, bid_t exec_block, eid_t *&beg_pos, vid_t *&csr, WalkManager &walk_manager ){
            //get current time in microsecond as seed to compute rand_r
            tid_t threadid = omp_get_thread_num();
            WalkDataType nowwalk = walk;
            vid_t sourId = walk_manager.getSourceId(nowwalk);
            vid_t curId = walk_manager.getCurrentId(nowwalk) + blocks[exec_block];
            vid_t dstId = curId;
            hid_t hop = walk_manager.getHop(nowwalk);
            // unsigned seed = (unsigned)std::chrono::high_resolution_clock::now().time_since_epoch().count();
            unsigned seed = walk+curId+hop+(unsigned)time(NULL);
            while (dstId >= blocks[exec_block] && dstId < blocks[exec_block+1] ){
                updateInfo(sourId, dstId, threadid, hop);
                vid_t dstIdp = dstId - blocks[exec_block];
                eid_t outd = beg_pos[dstIdp+1] - beg_pos[dstIdp];
                if (outd > 0 && (float)rand_r(&seed)/RAND_MAX > 0.15 ){
                    eid_t pos = beg_pos[dstIdp] - beg_pos[0] + ((eid_t)rand_r(&seed))%outd;
                    dstId = csr[pos];
                }else{
                    dstId = sourId;
                }
                hop++;
                nowwalk++;
                if(hop%L == L-1) break;
            }
            if( hop%L != L-1 ){
                bid_t p = getblock( dstId );
                if(p>=nblocks) return;
                walk_manager.moveWalk(nowwalk, p, threadid, dstId - blocks[p]);
                walk_manager.setMinStep( p, hop );
                walk_manager.ismodified[p] = true;
            }
    }
};

#endif