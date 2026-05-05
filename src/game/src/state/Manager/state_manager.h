#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include "bn_vector.h"
#include "bn_array.h"
#include "state/state_id.h"

// 前方宣言
class State;
#include "state/shared_context.h"

class StateManager {
public:
    StateManager();

    // 起動時に全Stateを登録するための関数
    void register_state(StateID id, State* state);

    // --- 遷移リクエスト関数（Stateのupdate内から呼ばれる） ---
    // 現在の画面を終了し、新しい画面へ
    void change_state(StateID id);
    // 現在の画面を一時停止したまま、上に新しい画面を乗せる（ポーズ画面など）
    void push_state(StateID id);
    // 上に乗っている画面を終了し、下の画面を再開する
    void pop_state();

    // メインループから毎フレーム呼ぶ関数
    void update(SharedContext& ctx);

private:
    // 予約された遷移リクエストを実際に処理する関数
    void process_requests(SharedContext& ctx);

    // 登録された全Stateへのポインタを保持する配列
    bn::array<State*, static_cast<int>(StateID::COUNT)> registry_;
    
    // 現在アクティブなStateのスタック
    bn::vector<State*, 8> stack_;

    // 遷移リクエストの種類
    enum class RequestType { NONE, CHANGE, PUSH, POP };
    RequestType current_request_;
    StateID requested_state_id_;
};

#endif // STATE_MANAGER_H