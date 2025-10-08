
## Message Window Configuration

- Siglus prefix: `MWND.xxx`
- Reallive prefix: `WINDOW.xxx`

| Reallive          | Siglus                       | Annotation                            |
|:-----------------:|:----------------------------:|:-------------------------------------:|
| `ATTR_MOD`        | ?                            | `use_window_attr`                     |
| `ATTR`            | `FUCHI_COLOR`?               | `r,g,b,a,filter`                      |
| `MOJI_SIZE`       | `MOJI_SIZE`                  | `default_font_size_in_pixels`         |
| `MOJI_CNT`        | `MOJI_CNT`                   | `window_size_in_chars(x,y)`           |
| `MOJI_REP`        | `MOJI_SPACE`                 | `spacing(x,y)`                        |
| `LUBY_SIZE`       | `RUBY_SIZE`                  | Size of the furigana `ruby_size`      |
| `MOJI_POS`        | `MESSAGE_MARGIN`             | `box_padding(up,down,left,right)`     |
| `POS`             | `WINDOW_POS`, `MESSAGE_POS`? | `origin, x_offset, y_offset`          |
| `INDENT_USE`      | ?                            | `use_indentation = true`              |
| `KEYCUR_MOD`      | ?                            | `keycursor_type, pos_x, pos_y`        |
| `R_COMMAND_MOD`   | ?                            | Should insert new paragraph on pause? |
| `WAKU_SETNO`      | `WAKU_NO`                    | Main text window's waku               |
| `NAME_MOD`        | `NAME_DISP_MODE`             | Enable name window? `name_mod`        |
| `NAME_WAKU_SETNO` | ?                            | `name_waku_no`                        |
| `NAME_MOJI_REP`   | ?                            | `name_x_spacing`                      |
| `NAME_MOJI_POS`   | ?                            | namebox padding(x,y)                  |
| `NAME_POS`        | ?                            | namebox offset(x,y)                   |
| `NAME_WAKU_DIR`   | ?                            | `name_waku_dir_set = 0`               |
| `NAME_CENTERING`  | ?                            | `namebox_centering = 0`               |
| `NAME_MOJI_MIN`   | ?                            | `minimum_namebox_size = 4`            |
| `NAME_MOJI_SIZE`  | `NAME_MOJI_SIZE`             | `name_size`                           |

- Name textbox is avaliable if `name_mod==1` and `name_waku_no` is valid

### Face information

TODO

## Text Waku Configuration

Siglus & Reallive prefix: `WAKU.xxx`

### Reallive Normal Waku

| Reallive         | Annotation                     |
|:----------------:|:------------------------------:|
| `NAME`           | `waku_main`                    |
| `BACK`           | `waku_backing`                 |
| `BTN`            | `waku_button`                  |
| `CLEAR_BOX`      | Toggle interface hidden button |
| `MSGBKLEFT_BOX`  | Back page button               |
| `MSGBKRIGHT_BOX` | Forward page button            |
| `READJUMP_BOX`   | Toggle skip mode button        |
| `AUTOMODE_BOX`   | Toggle auto mode button        |

TODO: Exbtn

### Reallive type-4 Waku

| Reallive | Annotation                    |
|:--------:|:-----------------------------:|
| `NAME`   | `waku_main`                   |
| `AREA`   | `area(top,bottom,left,right)` |
