; Optimized start routine: Provides a single drop-in script
; for an optimized startup routine. Faster, safer, & easier
;===== machine: A1 Mini ===================================
;===== date: 20251111 =====================================
;===== version: 1.3.4 =====================================
;===== modified by: Cascade Media LLC =====================
;===== changelog: =========================================
;===== 1.3.4 - Disabled vibration compensation due to eddy sensor warnings
;===== 1.3.3 - Enabled quick vibration compensation
;===== 1.3.2 - Disabled vibration compensation
;            - Shows signs of underextrusion when enabled
;===== 1.3.1 - Adds quick version of vibration compensation
;===== 1.3   - Adds 0.2mm and 0.6mm nozzle support
;            - Enhances nozzle temperature handling
;            - Updates guarded movements for safety
;            - Adds mech-mode (vibration) optional check
;            - Improved accuracy on stabilization pattern
;===== 1.2   - Optimized all paths
;===== 1.1   - Release for public
;===== 1.0   - Initial version

;===== start warm-up sequence =============================
M1002 gcode_claim_action : 2    ; status: heating
M1002 set_filament_type:{filament_type[initial_no_support_extruder]}

; conservative nozzle preheat based on material
M104 S{nozzle_temperature_initial_layer[initial_extruder] - 50}
M140 S{bed_temperature_initial_layer_single}

G392 S0                         ; disable clog detect
M9833.2                         ; bambu: set noise/mech params

;===== start printer sound ================================
; remove sounds for quieter startup
;castle-complete
;music_long: 5.837832
M17
M400 S1
M1006 S1
M1006 L84 M70 N99
M1006 A41 B8 L68 C25 D8 M35 E49 F8 N31 
M1006 A0 B8 C25 D8 M35 E49 F8 N31 
M1006 A37 B8 L64 C25 D8 M35 E44 F8 N38 
M1006 A0 B8 C25 D8 M35 E44 F8 N38 
M1006 A32 B8 L42 C25 D9 M35 E41 F9 N38 
M1006 A0 B8 C25 D8 M35 E41 F7 N38 
M1006 A41 B8 L68 C25 D8 M35 E49 F8 N31 
M1006 A0 B8 C25 D8 M35 E49 F8 N31 
M1006 A37 B8 L64 C25 D8 M35 E44 F8 N38 
M1006 A0 B8 C25 D8 M35 E44 F8 N38 
M1006 A32 B8 L42 C25 D8 M35 E41 F8 N38 
M1006 A0 B8 C25 D8 M35 E41 F8 N38 
M1006 A41 B24 L68 C25 D24 M35 E49 F25 N31 
;Tick 122, Time 1 sec
M73 P17 R0
M1006 A41 B8 L68 C25 D8 M35 E49 F8 N31 
M1006 A41 B8 L68 C25 D8 M35 E49 F8 N31 
M1006 A41 B8 L68 C25 D8 M35 E49 F8 N31 
M1006 A41 B8 L68 C25 D8 M35 E49 F8 N31 
M1006 A0 B8 C0 D8 E49 F8 N31 
M1006 A41 B8 L68 C25 D8 M35 E49 F8 N31 
M1006 A0 B8 C0 D8 E49 F8 N31 
M1006 A41 B8 L68 C25 D8 M35 E49 F8 N31 
M1006 A0 B9 C0 D9 E49 F8 N31 
M1006 A42 B8 L79 C26 D8 M28 E50 F8 N52 
;Tick 203, Time 2 sec
M73 P34 R0
M1006 A0 B8 C26 D8 M28 E50 F8 N52 
M1006 A38 B8 L61 C26 D8 M28 E45 F8 N45 
M1006 A0 B8 C26 D8 M28 E45 F8 N45 
M1006 A33 B8 L56 C26 D8 M28 E42 F8 N38 
M1006 A0 B8 C26 D8 M28 E42 F8 N38 
M1006 A42 B8 L79 C26 D8 M28 E50 F8 N52 
M1006 A0 B8 C26 D8 M28 E50 F8 N52 
M1006 A38 B8 L61 C26 D9 M28 E45 F9 N45 
M1006 A0 B8 C26 D8 M28 E45 F7 N45 
M1006 A33 B8 L56 C26 D8 M28 E42 F8 N38 
M1006 A0 B8 C26 D8 M28 E42 F8 N38 
M1006 A42 B24 L79 C26 D24 M28 E50 F24 N52 
;Tick 316, Time 3 sec
M73 P51 R0
M1006 A42 B8 L79 C26 D8 M28 E50 F8 N52 
M1006 A42 B8 L79 C26 D8 M28 E50 F8 N52 
M1006 A42 B8 L79 C26 D8 M28 E50 F9 N52 
M1006 A42 B8 L79 C26 D8 M28 E50 F8 N52 
M1006 A0 B8 C0 D8 E50 F8 N52 
M1006 A42 B8 L79 C26 D8 M28 E50 F8 N52 
M1006 A0 B8 C0 D8 E50 F8 N52 
M1006 A42 B8 L79 C26 D8 M28 E50 F8 N52 
M1006 A0 B8 C0 D8 E50 F8 N52 
M1006 A44 B8 L83 C35 D8 M59 E52 F8 N69 
M1006 A0 B8 C0 D8 E52 F8 N69 
;Tick 405, Time 4 sec
M73 P68 R0
M1006 A40 B8 L71 C32 D8 M45 E47 F9 N45 
M1006 A0 B8 C0 D8 E47 F7 N45 
M1006 A35 B8 L50 C28 D8 M21 E44 F8 N38 
M1006 A0 B8 C0 D8 E44 F8 N38 
M1006 A44 B8 L83 C35 D8 M59 E52 F8 N69 
M1006 A0 B8 C0 D8 E52 F8 N69 
M1006 A40 B8 L71 C32 D8 M45 E47 F8 N45 
M1006 A0 B8 C0 D8 E47 F8 N45 
M1006 A35 B8 L50 C28 D8 M21 E44 F8 N38 
M1006 A0 B8 C0 D8 E44 F8 N38 
M1006 A44 B24 L83 C35 D24 M59 E52 F24 N69 
;Tick 511, Time 5 sec
M73 P85 R0
M1006 A44 B8 L83 C35 D8 M59 E52 F8 N69 
M1006 A44 B8 L83 C35 D8 M59 E52 F8 N69 
M1006 A44 B8 L83 C35 D8 M59 E52 F8 N69 
M1006 A46 B8 L83 C37 D8 M69 E54 F8 N69 
M1006 A0 B8 C0 D8 E0 F8 
M1006 A46 B8 L83 C37 D8 M69 E54 F8 N69 
M1006 A0 B9 C0 D9 E0 F9 
M1006 A46 B8 L83 C37 D8 M69 E54 F8 N69 
M1006 A0 B8 C0 D8 E0 F8 
M1006 A48 B97 L83 C39 D97 M69 E56 F97 N49 
M1006 W
M18

;===== initialize machine status ==========================
G90                             ; absolute positioning
M83                             ; relative extrusion
M211 X1 Y1 Z1                   ; enable soft endstops

M630 S0 P0                      ; bambu: reset internal state
M204 S6000                      ; set default acceleration
M17 X0.7 Y0.9 Z0.5              ; set motor current
M960 S5 P1                      ; enable toolhead lamp

M220 S100                       ; feedrate to 100%
M221 S100                       ; flowrate to 100%
M982.2 S1                       ; enable cog noise reduction
M975 S1                         ; enable motion gating
M106 P1 S0                      ; disable fan while heating
M73.2 R1.0                      ; bambu: reset time left magnitude

;===== home and stage printer =============================
M17                             ; enable motors
G28 X Y                         ; safely home X and Y
G0 X55 Y175 F10000              ; move to safe place to home Z
G380 S2 Z5 F1200                ; guarded clearance before homing
G28 Z P0 T300                   ; safely home Z
M400                            ; wait for movements

M211 S                          ; push endstop status
M211 X0 Y0 Z0                   ; disable soft endstop
G1 Z5 F2000                     ; add clearance before move
G1 X0 F10000                    ; move to service area
G1 X-13.5 F3000                 ; move into wiper
M400

;===== switch material in AMS =============================
M620 M                          ; enable remap
M620 S[initial_no_support_extruder]A
    G392 S1                     ; enable clog detect
    M1002 gcode_claim_action : 4
    M400
    M1002 set_filament_type:UNKNOWN
    M109 S[nozzle_temperature_initial_layer]
    M104 S250
    M400
    T[initial_no_support_extruder]
    G1 X-13.5 F3000
    M400
    M620.1 E F{filament_max_volumetric_speed[initial_no_support_extruder]/2.4053*60} T{nozzle_temperature_range_high[initial_no_support_extruder]}
    M109 S250                   ; set nozzle to common flush temp
    M106 P1 S0
    G92 E0
    G1 E50 F200
    M400
    M1002 set_filament_type:{filament_type[initial_no_support_extruder]}
    M104 S{nozzle_temperature_range_high[initial_no_support_extruder]}
    G92 E0
    G1 E50 F{filament_max_volumetric_speed[initial_no_support_extruder]/2.4053*60}
    M400
    M106 P1 S178
    G92 E0
    G1 E5 F{filament_max_volumetric_speed[initial_no_support_extruder]/2.4053*60}
    M109 S{nozzle_temperature_initial_layer[initial_no_support_extruder]-20}
    M104 S{nozzle_temperature_initial_layer[initial_no_support_extruder]-40}
    G92 E0
    G1 E-0.5 F300

    G1 X0 F20000
    G1 X-13.5 F3000
    G1 X0 F20000
    G1 X-13.5 F3000
    G1 X0 F12000
    G1 X-13.5 F3000
    
    ; reset nozzle to expected temperature
    M104 S{nozzle_temperature_initial_layer[initial_no_support_extruder] - 50}               
    G392 S0                     ; disable clog detect
M621 S[initial_no_support_extruder]A

;===== build plate detection (flagged) ====================
M1002 judge_flag build_plate_detect_flag
M622 S1
    G39.4                         ; bambu: quick build plate detection
    M400

    ; quick mech-mode resonance check for improving vertical artifacts
    ; runs ONLY when "Bed Leveling" is enabled
    ; uses vibration amplitude to work on all nozzles
    ; @warning: reliably triggers the eddy current sensor warning, testing alternatives to the long and loud sweeps from the default routine
    ; G1 X128 Y128 Z10 F20000       ; use bed center for symetrical testing
    ; M400 P200
    ; M970.3 Q1 A7 B30 C80 H12 K0 O2
    ; M974 Q1 S2 P0
M623

;===== clean nozzle =======================================
M1002 gcode_claim_action : 14   ; status: nozzle cleaning

G90
M83
M106 P1 S255                    ; short blast to neck any strands
G4 P800
M106 P1 S0                      ; keep fan off during heating

; perform a short knock sequence by bending
; oozed filament before brushing sequence
G1 E-1.0 F500                   ; small retract before taps
G1 Z5 F2000                     ; clearance
G0 X90 Y-4 F10000               ; move to the purge area
G380 S3 Z-2 F1200               ; gentle tap on plate
G1 Z2 F2000
G1 X91 F3000
G380 S3 Z-2 F1200
G1 Z2 F2000
G1 X92 F3000
G380 S3 Z-2 F1200

; brush material on rubber
G1 Z5 F2000
G1 X25 Y176 F20000
G1 Z0.2 F2000
G1 Y186
G91
G1 X-30 F20000
G1 Y-2
G1 X27
G1 Y1.5
G1 X-28
G1 Y-2
G1 X30
G1 Y1.5
G1 X-30
G90
G1 Z5 F2000

; brush material on rubber, slight offset
G1 Z5 F2000
G1 X25 Y177 F20000
G1 Z0.2 F2000
G1 Y187
G91
G1 X-30 F20000
G1 Y-2
G1 X27
G1 Y1.5
G1 X-28
G1 Y-2
G1 X30
G1 Y1.5
G1 X-30
G90
G1 Z5 F2000

; restore protections and establish raw Z reference
M211 R                          ; restore softend status
G29.2 S0                        ; disable ABL for raw Z
G0 X55 Y175 F10000
G28 Z P0 T300
G29.2 S1                        ; enable ABL

;===== park and wait for heating ==========================
M1002 gcode_claim_action : 2    ; status: heating
G1 Z5 F2000                     ; ensure clearance
G1 X10 Y10 F10000               ; park clear of brush

; set and wait for bed to final temperature
M140 S[bed_temperature_initial_layer_single]
M104 S{nozzle_temperature_initial_layer[initial_extruder] - 50}
M190 S[bed_temperature_initial_layer_single]

;===== bed leveling (flagged) =============================
M1002 judge_flag g29_before_print_flag
M622 J1
    G29.2 S0                    ; disable ABL for probing
    M1002 gcode_claim_action : 1 ; status: auto bed leveling
    G29 A1 X{first_layer_print_min[0]} Y{first_layer_print_min[1]} I{first_layer_print_size[0]} J{first_layer_print_size[1]}
    M400
    M500                        ; save mesh
    G29.2 S1                    ; enable ABL with fresh mesh
M623

;===== bed leveling (not flagged) =========================
M1002 judge_flag g29_before_print_flag
M622 J0
    M1002 gcode_claim_action : 13 ; status: homing toolhead
    G28 T300                    ; permissive temp home
    G29.2 S1                    ; enable ABL with existing mesh
M623

;===== nozzle load line ===================================
M1002 gcode_claim_action : 2    ; status: heating
M975 S1                         ; enable motion gating (explicit)
G90                             ; re-assert positioning (explicit)
M83
T1000                           ; select local tool

M211 S                          ; push endstop status
M211 X0 Y0 Z0                   ; disable soft endstop
G1 Z5 F2000                     ; add clearance before move
G1 X0 F10000                    ; move to service area
G1 X-13.5 F3000                 ; move into wiper

; minimal prime with micro-retract inside service area
G92 E0                          ; reset extruded amount before line
G1 E1.2 F500
G1 E-0.25 F1500

; set and wait for nozzle to final temperature
M104 S{nozzle_temperature_initial_layer[initial_extruder]}
M109 S{nozzle_temperature_initial_layer[initial_extruder]}

;===== prepare sensors for calibration ====================
M1002 set_filament_type:UNKNOWN ; prepare filament for calibration
M412 S1                         ; enable filament runout detect
M400
M620.3 W1                       ; enable filament tangle detect
G392 S0                         ; disable clog detect during calibration
M400 S2 P100                    ; small wait with sensor settle

;===== flow dynamics calibration (flagged) ================
M1002 set_filament_type:{filament_type[initial_no_support_extruder]}
M1002 judge_flag extrude_cali_flag
M622 J1
    M1002 gcode_claim_action : 8 ; status: calibrating extrusion
    
    M900 K0.0 L1000.0 M1.0      ; pressure advance baseline
    G90
    M83                         ; re-assert positioning (explicit)

    G1 Z5 F2000                 ; safe lift
    G0 X68 Y-4 F10000           ; move near start position
    G0 Z0.3 F2000               ; move to start position
    M400

    G0 X88 E10 F{outer_wall_volumetric_speed/(24/20)*60}
    G0 X93 E.3742 F{outer_wall_volumetric_speed/(0.3*0.5)/4*60}
    G0 X98 E.3742 F{outer_wall_volumetric_speed/(0.3*0.5)*60}
    G0 X103 E.3742 F{outer_wall_volumetric_speed/(0.3*0.5)/4*60}
    G0 X108 E.3742 F{outer_wall_volumetric_speed/(0.3*0.5)*60}
    G0 X113 E.3742 F{outer_wall_volumetric_speed/(0.3*0.5)/4*60}
    G0 Y0 Z0 F20000             ; finish the patterned sweep
    M400
    
    G1 X-13.5 Y0 Z10 F10000     ; park in service area
    M400

    ; primary dynamic extrusion compensation
    G1 E10 F{outer_wall_volumetric_speed/2.4*60}
    M983 F{outer_wall_volumetric_speed/2.4} A0.3 H[nozzle_diameter]
    M106 P1 S180                ; enable fan to neck strand
    M400 S7                     ; short settle
    
    G1 X0 F20000                ; wipe & shake
    G1 X-13.5 F3000
    G1 X0 F20000
    G1 X-13.5 F3000
    G1 X0 F12000
    G1 X-13.5 F3000
    M400
    M106 P1 S0                  ; disable fan
    
    ; retry once if needed
    M1002 judge_last_extrude_cali_success
    M622 J0
        M983 F{outer_wall_volumetric_speed/2.4} A0.3 H[nozzle_diameter]
        M106 P1 S180
        M400 S7
        G1 X0 F20000
        G1 X-13.5 F3000
        G1 X0 F20000
        G1 X-13.5 F3000
        G1 X0 F12000
        G1 X-13.5 F3000
        M400
        M106 P1 S0
    M623
    
    ; final corrections and cleanup
    G1 X-13.5 F3000
    M400
    M984 A0.1 E1 S1 F{outer_wall_volumetric_speed/2.4} H[nozzle_diameter]
    M106 P1 S180
    M400 S7
    G1 X0 F20000
    G1 X-13.5 F3000
    G1 X0 F20000
    G1 X-13.5 F3000
    G1 X0 F12000
    G1 X-13.5 F3000
    M400
    M106 P1 S0
M623                            ; end flow dynamics calibration
M211 R                          ; restore soft endstops status

;===== extrude calibration test ===========================
; hold first-layer temp and set modes (explicit)
M109 S{nozzle_temperature_initial_layer[initial_extruder]}
M190 S{bed_temperature_initial_layer_single}
G90
M83

; clear any ooze before calibrating
M106 P1 S180                    ; moderate fan for PLA and PETG
M400 S2
G1 E-0.05 F1500
G1 X0 F20000
G1 X-13.5 F3000
G1 X0 F20000
G1 X-13.5 F3000
G1 X0 F12000
G1 X-13.5 F3000
M106 P1 S0
M400

; draw short stabilization pattern
G1 Z5 F3000
G0 X68 Y-2.5 F20000
G0 Z0.3 F3000
G0 X88 E10 F{outer_wall_volumetric_speed/(24/20)*60}
G0 X93 E0.3742 F{outer_wall_volumetric_speed/(0.3*0.5)/4*60}
G0 X98 E0.3742 F{outer_wall_volumetric_speed/(0.3*0.5)*60}
G0 X103 E0.3742 F{outer_wall_volumetric_speed/(0.3*0.5)/4*60}
G0 X108 E0.3742 F{outer_wall_volumetric_speed/(0.3*0.5)*60}
G0 X113 E0.3742 F{outer_wall_volumetric_speed/(0.3*0.5)/4*60}
G0 X115 Z0 F20000
G1 Z5 F3000
M400

;===== for textured pei plate =============================
{if curr_bed_type=="Textured PEI Plate"}
    G29.1 Z{-0.02}
{endif}

;===== normalize lights/fans & re-enable protections =====
M960 S1 P0                      ; light/laser ch1 off
M960 S2 P0                      ; light/laser ch2 off
M960 S5 P0                      ; toolhead lamp off
M106 P1 S0                      ; part fan off
M106 P2 S0                      ; aux fan off
M106 P3 S0                      ; chamber fan off

G392 S1                         ; re-enable clog detect
G29.2 S1                        ; enable ABL for print

;===== final staging =====================================
M1002 gcode_claim_action : 0    ; status: printing
M975 S1                         ; keep vibration suppression on
G90
M83
T1000
M211 R                          ; restore endstop status
M211 X0 Y0 Z0                   ; disable soft endstops
M1007 S1                        ; bambu: keep enabled

; ===== hand-off to slicer first move =====================