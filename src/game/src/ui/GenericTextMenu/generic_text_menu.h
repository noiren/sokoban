#ifndef GENERIC_TEXT_MENU_H
#define GENERIC_TEXT_MENU_H

#include "bn_array.h"
#include "bn_fixed.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_string.h"
#include "bn_string_view.h"
#include "bn_vector.h"

// 上下キーで選択し、決定時に「値」(int) を返す汎用テキストリスト＋枠。
// ギャラリー・選択肢などで再利用する。スプライトは draw のたびに out_sprites へ追加する。
class GenericTextMenu {
public:
    static constexpr int kMaxItems = 40;
    static constexpr int kMaxVisible = 8;

    enum class Poll : uint8_t {
        None,
        Moved,       // カーソル移動（再描画推奨）
        Confirmed,   // 決定（*out_value に値）
        Cancelled    // B（戻る）
    };

    GenericTextMenu();

    // 表示位置: 左揃えテキストの基準 X、先頭行の Y、行間 dy、一度に見せる最大行数
    void configure(bn::fixed anchor_x, bn::fixed first_y, bn::fixed line_dy, int max_visible);

    void clear_items();
    // ラベルは短め（概ね 40 バイト未満）を推奨。満杯なら false。
    bool push_item(int value, bn::string_view label);

    int item_count() const { return count_; }
    int cursor_index() const { return cursor_; }
    void set_cursor(int index);

    // 1 フレームにつき 1 回。決定時のみ *out_value を設定。
    Poll poll(int* out_value);

    // 既存の vector をクリアせず末尾に追加する（先にタイトル等を描いた場合用）
    void draw(bn::sprite_text_generator& gen, bn::vector<bn::sprite_ptr, 160>& out_sprites);

private:
    struct Entry {
        int value = 0;
        bn::string<48> label;
    };

    void _clamp_cursor();
    void _clamp_scroll();

    bn::array<Entry, kMaxItems> entries_;
    int count_ = 0;
    int cursor_ = 0;
    int scroll_ = 0; // 先頭に表示する items[] のインデックス

    bn::fixed anchor_x_;
    bn::fixed first_y_;
    bn::fixed line_dy_;
    int max_visible_ = 8;
};

#endif
