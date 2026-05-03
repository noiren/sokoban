#include "state_manager.h"

void StateManager::push(State* state) {
    stack_.push_back(state);
    state->on_enter(*this);
    state->init(*this);
}

void StateManager::pop() {
    if (!stack_.empty()) {
        stack_.back()->shutdown();
        stack_.back()->on_exit(*this);
        stack_.pop_back();
    }
}

void StateManager::replace(State* state) {
    if (!stack_.empty()) {
        stack_.back()->shutdown();
        stack_.back()->on_exit(*this);
        stack_.pop_back();
    }
    stack_.push_back(state);
    state->on_enter(*this);
    state->init(*this);
}

void StateManager::update() {
    if (!stack_.empty()) {
        stack_.back()->update(*this);
    }
}

State* StateManager::current() {
    if (!stack_.empty()) {
        return stack_.back();
    }
    return nullptr;
}

bool StateManager::empty() const {
    return stack_.empty();
}
