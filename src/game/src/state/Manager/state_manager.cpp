#include "state/Manager/state_manager.h"
#include "state/state.h"
#include "bn_assert.h"
#include "audio/sound_manager.h"

StateManager::StateManager() 
    : current_request_(RequestType::NONE),
      requested_state_id_(StateID::NONE) 
{
    registry_.fill(nullptr);
}

void StateManager::register_state(StateID id, State* state) {
    int index = static_cast<int>(id);
    BN_ASSERT(index >= 0 && index < static_cast<int>(StateID::COUNT), "Invalid StateID");
    registry_[index] = state;
}

void StateManager::change_state(StateID id) {
    current_request_ = RequestType::CHANGE;
    requested_state_id_ = id;
}

void StateManager::push_state(StateID id) {
    current_request_ = RequestType::PUSH;
    requested_state_id_ = id;
}

void StateManager::pop_state() {
    current_request_ = RequestType::POP;
}

void StateManager::update(SharedContext& ctx) {
    // 1. 前回フレームで予約された遷移リクエストがあれば、まずそれを処理する
    process_requests(ctx);

    // 2. 現在スタックの一番上にある State を更新する
    if (!stack_.empty()) {
        stack_.back()->update(*this, ctx);
    }
}

void StateManager::process_requests(SharedContext& ctx) {
    if (current_request_ == RequestType::NONE) {
        return;
    }

    // 処理中のリクエストをローカルにコピーし、メンバ変数をリセットする。
    // こうすることで、enter() や exit() 等の中で新たなリクエストが
    // 発行されても上書き消去されず、次フレームで処理されるようになる。
    RequestType req = current_request_;
    StateID req_id = requested_state_id_;
    
    current_request_ = RequestType::NONE;
    requested_state_id_ = StateID::NONE;

    State* next_state = nullptr;
    if (req_id != StateID::NONE) {
        next_state = registry_[static_cast<int>(req_id)];
        BN_ASSERT(next_state != nullptr, "Requested State is not registered!");
    }

    switch (req) {
        case RequestType::CHANGE:
            if (!stack_.empty()) {
                stack_.back()->exit(*this, ctx);
                SoundManager::instance().stop_bgm(0); // Stateを出るときにBGMを停止
                stack_.pop_back();
            }
            stack_.push_back(next_state);
            next_state->enter(*this, ctx);
            break;

        case RequestType::PUSH:
            if (!stack_.empty()) {
                stack_.back()->pause(*this, ctx);
            }
            stack_.push_back(next_state);
            next_state->enter(*this, ctx);
            break;

        case RequestType::POP:
            if (!stack_.empty()) {
                stack_.back()->exit(*this, ctx);
                SoundManager::instance().stop_bgm(0); // Stateを出るときにBGMを停止
                stack_.pop_back();
            }
            if (!stack_.empty()) {
                stack_.back()->resume(*this, ctx);
            }
            break;

        default:
            break;
    }
}