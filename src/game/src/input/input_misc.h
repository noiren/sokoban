#ifndef INPUT_MISC_H
#define INPUT_MISC_H

namespace InputMisc
{
    // 押しっぱなし後、リピートが始まるまでの待機フレーム数
    static constexpr int REPEAT_DELAY    = 20;

    // リピート中の入力間隔（フレーム）
    static constexpr int REPEAT_INTERVAL = 6;
}

#endif // INPUT_MISC_H
