;====TOP-AMS====A1mini====2025年1月18日14:42:50====start=====

{if toolchange_count > 1}
G17
G2 Z{max_layer_z + 0.4} I0.86 J0.86 P1 F10000 ;回抽抬升
;endif;放在结尾,目前设定首次就不换色

G1 Z{max_layer_z + 3.0}
;Z抬升


{if long_retractions_when_cut[previous_extruder]}
G1 E-{retraction_distances_when_cut[previous_extruder]} F1200;不太理解这个1200的取值
{endif}
M400
;长回抽

G1 X-3.5 F18000
G1 X-13.5 F3000;移动到马桶(A1mini坐标)

M140 S{next_extruder + 1};//使用热床目标温度传递通道信息
M400 U1;//暂停



;下面为原版冲刷,加大了刷料时的风扇速度 S178->S255

{if flush_length_1 > 1}
;FL1 > 1
; FLUSH_START
; always use highest temperature to flush
M400
M1002 set_filament_type:UNKNOWN
M109 S[nozzle_temperature_range_high]
;M109 S[]
M106 P1 S60
{if flush_length_1 > 23.7}
G1 E23.7 F{old_filament_e_feedrate} ; do not need pulsatile flushing for start part
G1 E{(flush_length_1 - 23.7) * 0.02} F50
G1 E{(flush_length_1 - 23.7) * 0.23} F{old_filament_e_feedrate}
G1 E{(flush_length_1 - 23.7) * 0.02} F50
G1 E{(flush_length_1 - 23.7) * 0.23} F{new_filament_e_feedrate}
G1 E{(flush_length_1 - 23.7) * 0.02} F50
G1 E{(flush_length_1 - 23.7) * 0.23} F{new_filament_e_feedrate}
G1 E{(flush_length_1 - 23.7) * 0.02} F50
G1 E{(flush_length_1 - 23.7) * 0.23} F{new_filament_e_feedrate}
{else}
G1 E{flush_length_1} F{old_filament_e_feedrate}
{endif}
; FLUSH_END
G1 E-[old_retract_length_toolchange] F1800
G1 E[old_retract_length_toolchange] F300
M400
M1002 set_filament_type:{filament_type[next_extruder]}
{endif}

{if flush_length_1 > 45 && flush_length_2 > 1}
;FL1>45 && FL2>1
; WIPE
M400
M106 P1 S255
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
;FL2>1
M106 P1 S60
; FLUSH_START
G1 E{flush_length_2 * 0.18} F{new_filament_e_feedrate}
G1 E{flush_length_2 * 0.02} F50
G1 E{flush_length_2 * 0.18} F{new_filament_e_feedrate}
G1 E{flush_length_2 * 0.02} F50
G1 E{flush_length_2 * 0.18} F{new_filament_e_feedrate}
G1 E{flush_length_2 * 0.02} F50
G1 E{flush_length_2 * 0.18} F{new_filament_e_feedrate}
G1 E{flush_length_2 * 0.02} F50
G1 E{flush_length_2 * 0.18} F{new_filament_e_feedrate}
G1 E{flush_length_2 * 0.02} F50
; FLUSH_END
G1 E-[new_retract_length_toolchange] F1800
G1 E[new_retract_length_toolchange] F300
{endif}

{if flush_length_2 > 45 && flush_length_3 > 1}
;FL2>45 &&FL3>1
; WIPE
M400
M106 P1 S255
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
;FL3>1
M106 P1 S60
; FLUSH_START
G1 E{flush_length_3 * 0.18} F{new_filament_e_feedrate}
G1 E{flush_length_3 * 0.02} F50
G1 E{flush_length_3 * 0.18} F{new_filament_e_feedrate}
G1 E{flush_length_3 * 0.02} F50
G1 E{flush_length_3 * 0.18} F{new_filament_e_feedrate}
G1 E{flush_length_3 * 0.02} F50
G1 E{flush_length_3 * 0.18} F{new_filament_e_feedrate}
G1 E{flush_length_3 * 0.02} F50
G1 E{flush_length_3 * 0.18} F{new_filament_e_feedrate}
G1 E{flush_length_3 * 0.02} F50
; FLUSH_END
G1 E-[new_retract_length_toolchange] F1800
G1 E[new_retract_length_toolchange] F300
{endif}

{if flush_length_3 > 45 && flush_length_4 > 1}
;FL3>45 && FL4>1
; WIPE
M400
M106 P1 S255
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
;FL>4
M106 P1 S60
; FLUSH_START
G1 E{flush_length_4 * 0.18} F{new_filament_e_feedrate}
G1 E{flush_length_4 * 0.02} F50
G1 E{flush_length_4 * 0.18} F{new_filament_e_feedrate}
G1 E{flush_length_4 * 0.02} F50
G1 E{flush_length_4 * 0.18} F{new_filament_e_feedrate}
G1 E{flush_length_4 * 0.02} F50
G1 E{flush_length_4 * 0.18} F{new_filament_e_feedrate}
G1 E{flush_length_4 * 0.02} F50
G1 E{flush_length_4 * 0.18} F{new_filament_e_feedrate}
G1 E{flush_length_4 * 0.02} F50
; FLUSH_END
{endif}

;M629//不知道这什么用

M400
M106 P1 S60
M109 S[new_filament_temp]
G1 E5 F{new_filament_e_feedrate} ;Compensate for filament spillage during waiting temperature
M400
G92 E0
G1 E-[new_retract_length_toolchange] F1800
M400
M106 P1 S255
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



{endif};if toolchange_count > 1
;====TOP-AMS====end=====