#ifndef STATE_H
#define STATE_H

class StateManager;
#include "state/shared_context.h"
enum class PhaseStep { OPENING, RUNNING, CLOSING };

class State {
public:
    virtual ~State();

    // 状態が開始された（スタックに積まれた）時に呼ばれる
    virtual void enter(StateManager& sm, SharedContext& ctx);

    // 毎フレーム呼ばれる（派生クラスでの実装を強制するため純粋仮想関数）
    virtual void update(StateManager& sm, SharedContext& ctx) = 0;

    // 状態が終了した（スタックから取り除かれる）時に呼ばれる
    virtual void exit(StateManager& sm, SharedContext& ctx);

    // 上に別のStateが積まれてバックグラウンドに回った時に呼ばれる
    virtual void pause(StateManager& sm, SharedContext& ctx);

    // 上のStateがなくなり、再びアクティブになった時に呼ばれる
    virtual void resume(StateManager& sm, SharedContext& ctx);
};

#endif // STATE_H