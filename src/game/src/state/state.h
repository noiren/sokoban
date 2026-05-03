#ifndef STATE_H
#define STATE_H

// Forward declaration
class StateManager;

// 各フェーズ共通のライフサイクルステップ
// OPENING : 入場処理中（フェードインなど）
// RUNNING : 通常更新中
// CLOSING : 退場処理中（フェードアウトなど）
enum class PhaseStep {
    OPENING,
    RUNNING,
    CLOSING,
};

// Abstract base class for all game states
class State {
public:
    virtual ~State() = default;

    // StateManager から push/replace されたとき（画面遷移で入ってきたとき）に呼ばれる
    // デフォルトは空実装。必要なら各Stateでオーバーライドする。
    virtual void on_enter(StateManager& /*manager*/) {}

    // StateManager から pop/replace されるとき（画面から出るとき）に呼ばれる
    // shutdown() の後に呼ばれる。デフォルトは空実装。
    virtual void on_exit(StateManager& /*manager*/) {}

    virtual void init(StateManager& manager) = 0;
    virtual void update(StateManager& manager) = 0;
    virtual void shutdown() = 0;
};

#endif // STATE_H
