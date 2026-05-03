#include "state_manager.h"

void StateManager::push(State* state) {
    // もし保留中のpopがあれば、先に処理する（安全策）
    if (pop_requested_) {
        pop_requested_ = false;
        stack_.back()->shutdown();
        stack_.back()->on_exit(*this);
        stack_.pop_back();
    }
    stack_.push_back(state);
    state->on_enter(*this);
    state->init(*this);
}

void StateManager::pop() {
    // 即座にポップせず、次回 update() のタイミングで行うよう予約する
    pop_requested_ = true;
}

void StateManager::replace(State* state) {
    // 保留中のpopがあればリセット
    pop_requested_ = false;
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

        // pop() が呼ばれていたら、State::update() の終了後に実際のポップ処理を行う
        // これにより、shutdown() した後に同じStateのコードが動き続けることを防ぐ
        if (pop_requested_) {
            pop_requested_ = false;
            stack_.back()->shutdown();
            stack_.back()->on_exit(*this);
            stack_.pop_back();
        }
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
