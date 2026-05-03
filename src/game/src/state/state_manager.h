#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include "bn_vector.h"
#include "state.h"

class StateManager {
public:
    void push(State* state);
    // pop() は即座にポップせず、次回 update() のタイミングで実際のポップ（shutdown + 削除）を行う
    void pop();
    void replace(State* state);
    void update();
    State* current();
    bool empty() const;

private:
    bn::vector<State*, 8> stack_;
    bool pop_requested_ = false;
};

#endif // STATE_MANAGER_H
