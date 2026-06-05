#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

enum class Action {
    MoveUp,
    MoveDown,
    MoveLeft,
    MoveRight,
    Decide,     // 決定 / 次へ : A, START
    Cancel,     // 戻る : B
    OpenMenu,   // メニュー : SELECT
    LButton,    // L ボタン
    RButton,    // R ボタン
    COUNT
};

class InputManager {
public:
    static InputManager& instance();

    void update();

    bool is_triggered(Action action) const;
    bool is_held(Action action)      const;
    bool is_released(Action action)  const;
    bool is_repeat(Action action)    const;

private:
    InputManager();

    static constexpr int ACTION_COUNT = static_cast<int>(Action::COUNT);
    int hold_frames_[ACTION_COUNT];

    static bool check_pressed(Action action);
    static bool check_held(Action action);
    static bool check_released(Action action);
};

#endif // INPUT_MANAGER_H
