#include "state.h"

// 仮想デストラクタのデフォルト実装
State::~State() = default;

// update以外は、派生クラスで実装しなくてもエラーにならないよう空のデフォルト実装を持たせる
void State::enter(StateManager& sm, SharedContext& ctx) {}
void State::exit(StateManager& sm, SharedContext& ctx) {}
void State::pause(StateManager& sm, SharedContext& ctx) {}
void State::resume(StateManager& sm, SharedContext& ctx) {}