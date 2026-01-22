; ===== machine: Bambu A1 mini =============
; ===== date: 20240613 =====================
; ===== Filamentor V0.3 ====================
; ===== Based on the official A1 mini change filament gcode (20231225)======

;G392 S0
;M1007 S0

; 实现：如果前后通道一致，直接返回，跳过所有换料操作；如果不一致，继续执行
{if next_extruder == previous_extruder || next_extruder >= 255}
; 前后通道一致或无效通道，直接返回，跳过所有换料操作
{else}
M140 S{next_extruder + 1};EXT

;M204 S9000
;{if toolchange_count > 1}
;G17
;G2 Z{max_layer_z + 0.4} I0.86 J0.86 P1 F10000 ; spiral lift a little from second lift
;{endif}
;G1 Z{max_layer_z + 3.0} F1200  

M400
M106 P1 S0
M106 P2 S0
;{if old_filament_temp > 142 && next_extruder < 255}
;M104 S[old_filament_temp]
;{endif}
G1 X-13.5 F18000
M400 U1


{if next_extruder < 255}
M400
G92 E0
{if flush_length_1 > 1}
; FLUSH_START
; always use highest temperature to flush
M400
M1002 set_filament_type:UNKNOWN
M106 P1 S60
{if flush_length_1 > 15}
G1 E15 F{old_filament_e_feedrate} ; 从23.7mm减少到15mm，减少初始冲洗
G1 E{(flush_length_1 - 15) * 0.02} F50
G1 E{(flush_length_1 - 15) * 0.23} F{old_filament_e_feedrate}
G1 E{(flush_length_1 - 15) * 0.02} F50
G1 E{(flush_length_1 - 15) * 0.23} F{new_filament_e_feedrate}
G1 E{(flush_length_1 - 15) * 0.02} F50
G1 E{(flush_length_1 - 15) * 0.23} F{new_filament_e_feedrate}
G1 E{(flush_length_1 - 15) * 0.02} F50
G1 E{(flush_length_1 - 15) * 0.23} F{new_filament_e_feedrate}
{else}
G1 E{flush_length_1} F{old_filament_e_feedrate}
{endif}
; FLUSH_END
G1 E-[old_retract_length_toolchange] F1800
G1 E[old_retract_length_toolchange] F300
M400
M1002 set_filament_type:{filament_type[next_extruder]}
{endif}

{if flush_length_1 > 60 && flush_length_2 > 1} ; 提高擦拭触发阈值，从45mm提高到60mm
; WIPE
M400
M106 P1 S178
M400 S3
G1 X-3.5 F18000
G1 X-13.5 F3000
G1 X-3.5 F18000
G1 X-13.5 F3000
G1 X-3.5 F18000
G1 X-13.5 F3000
M400
M106 P1 S0
{endif}

{if flush_length_2 > 1}
M106 P1 S60
; FLUSH_START
G1 E{flush_length_2 * 0.18} F{new_filament_e_feedrate}
G1 E{flush_length_2 * 0.02} F50
G1 E{flush_length_2 * 0.18} F{new_filament_e_feedrate}
G1 E{flush_length_2 * 0.02} F50
G1 E{flush_length_2 * 0.18} F{new_filament_e_feedrate}
G1 E{flush_length_2 * 0.02} F50
; FLUSH_END (从5次循环减少到3次)
G1 E-[new_retract_length_toolchange] F1800
G1 E[new_retract_length_toolchange] F300
{endif}

{if flush_length_2 > 60 && flush_length_3 > 1} ; 提高擦拭触发阈值，从45mm提高到60mm
; WIPE
M400
M106 P1 S178
M400 S3
G1 X-3.5 F18000
G1 X-13.5 F3000
G1 X-3.5 F18000
G1 X-13.5 F3000
G1 X-3.5 F18000
G1 X-13.5 F3000
M400
M106 P1 S0  
{endif}

{if flush_length_3 > 1}
M106 P1 S60
; FLUSH_START
G1 E{flush_length_3 * 0.18} F{new_filament_e_feedrate}
G1 E{flush_length_3 * 0.02} F50
G1 E{flush_length_3 * 0.18} F{new_filament_e_feedrate}
G1 E{flush_length_3 * 0.02} F50
G1 E{flush_length_3 * 0.18} F{new_filament_e_feedrate}
G1 E{flush_length_3 * 0.02} F50
; FLUSH_END (从5次循环减少到3次)
G1 E-[new_retract_length_toolchange] F1800
G1 E[new_retract_length_toolchange] F300
{endif}

{if flush_length_3 > 60 && flush_length_4 > 1} ; 提高擦拭触发阈值，从45mm提高到60mm
; WIPE
M400
M106 P1 S178
M400 S3
G1 X-3.5 F18000
G1 X-13.5 F3000
G1 X-3.5 F18000
G1 X-13.5 F3000
G1 X-3.5 F18000
G1 X-13.5 F3000
M400
M106 P1 S0
{endif}

{if flush_length_4 > 1}
M106 P1 S60
; FLUSH_START
G1 E{flush_length_4 * 0.18} F{new_filament_e_feedrate}
G1 E{flush_length_4 * 0.02} F50
G1 E{flush_length_4 * 0.18} F{new_filament_e_feedrate}
G1 E{flush_length_4 * 0.02} F50
G1 E{flush_length_4 * 0.18} F{new_filament_e_feedrate}
G1 E{flush_length_4 * 0.02} F50
; FLUSH_END (从5次循环减少到3次)
{endif}

{if toolchange_count == 1}
; For the first material change, FLUSH a bit more.
M400
M109 S250
M106 P1 S60

; FLUSH_START
G1 E20 F{old_filament_e_feedrate} ; 从50mm减少到20mm
; FLUSH_END

G1 E-[new_retract_length_toolchange] F1800
G1 E[new_retract_length_toolchange] F300
{endif}

M400
M106 P1 S60
M109 S[new_filament_temp]
G1 E2 F{new_filament_e_feedrate} ;Compensate for filament spillage during waiting temperature (从5mm减少到2mm)
M400
G92 E0
G1 E-[new_retract_length_toolchange] F1800
M400
M106 P1 S178
M400 S3
G1 X-3.5 F18000
G1 X-13.5 F3000
G1 X-3.5 F18000
G1 X-13.5 F3000
G1 X-3.5 F18000
G1 X-13.5 F3000
G1 X-3.5 F18000
G1 X-13.5 F3000
M400
G1 Z{max_layer_z + 3.0} F3000
M106 P1 S0
{if layer_z <= (initial_layer_print_height + 0.001)}
M204 S[initial_layer_acceleration]
{else}
M204 S[default_acceleration]
{endif}
{else}
G1 X[x_after_toolchange] Y[y_after_toolchange] Z[z_after_toolchange] F12000
{endif}
M620 S[next_extruder]A
T[next_extruder]
M621 S[next_extruder]A

G392 S0
M1007 S1
{endif}

