#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include "bn_vector.h"
#include "state.h"

class StateManager {
public:
    void push(State* state);
    void pop();
    void replace(State* state);
    void update();
    State* current();
    bool empty() const;

private:
    bn::vector<State*, 8> stack_;
};

#endif // STATE_MANAGER_H
