import json
import sys

def main():
    with open('tools/image_manifest.json', 'r') as f:
        manifest = json.load(f)

    includes = set()
    includes.add('#include "bn_regular_bg_items_bg_title_main.h"')
    includes.add('#include "bn_regular_bg_items_bg_menu_main.h"')
    
    mapping_code = []

    for img_set, items in manifest.items():
        mapping_code.append(f'            if (img_set == "{img_set}") {{')
        for img_no, path in items.items():
            # e.g. path = "ui/common/spr_selection_common" -> item = "spr_selection_common"
            item_name = path.split('/')[-1]
            includes.add(f'#include "bn_sprite_items_{item_name}.h"')
            
            mapping_code.append(f'                if (img_no == {img_no}) rs.sprite = bn::sprite_items::{item_name}.create_sprite(rs.x, rs.y);')
        mapping_code.append('            }')

    print("--- INCLUDES ---")
    for inc in sorted(list(includes)):
        print(inc)
        
    print("\n--- MAPPING ---")
    print("\n".join(mapping_code))

if __name__ == '__main__':
    main()
