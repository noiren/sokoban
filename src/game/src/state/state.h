#ifndef STATE_H
#define STATE_H

// Forward declaration
class StateManager;

// Abstract base class for all game states
class State {
public:
    virtual ~State() = default;
    virtual void init(StateManager& manager) = 0;
    virtual void update(StateManager& manager) = 0;
    virtual void shutdown() = 0;
};

#endif // STATE_H
