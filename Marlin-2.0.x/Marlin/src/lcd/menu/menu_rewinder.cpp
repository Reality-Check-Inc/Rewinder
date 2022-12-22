/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

//
// rEWINDER Menu
//

#include "../../inc/MarlinConfigPre.h"
//#include "src/MarlinCore.h"
#include "src/HAL/shared/Delay.h"
#include "src/module/planner.h" //rewinder
#include "src/module/printcounter.h" // PrintCounter or Stopwatch

#if HAS_LCD_MENU

#include "menu_item.h"
#include "src/lcd/ultralcd.h"

extern int Hit_Pos;
bool allow_job = false;
bool need_filament_first = false;
int filament_Length = ((Hit_Pos/2) * 10);
float wind_speed = REWINDER_MAX_SPEED;



static void Probe_Spool() {
  PGM_P const loadfilament = PSTR("Load filament to continue");
 if (READ(19)) {
   ui.set_status_P(loadfilament); 
   SERIAL_ECHOLN("WINDER..Need filament loaded"); 
   need_filament_first = true;
   BUZZ(30,30);
   delay(1000);
 }
  else {
    Hit_Pos = 0;
    need_filament_first = false;
    queue.inject_P(PSTR("G28 X\nG90\nM83\nG0 X90 F1500\nG0 X0")); //Home X, move to Max Bed (90) to sense spool size and move back to start position
    SERIAL_ECHOLN("WINDER..Probing spool");
    allow_job = false;
    encoderTopLine = 1; 
    encoderLine = 3;
    ui.screen_changed = true;
    ui.refresh();
    }
 }


static void Kill_Spool() {
  ui.defer_status_screen();
  SERIAL_ECHOLN("WINDER..Killing winding job NOW!");
  queue.clear();
  thermalManager.disable_all_heaters(); // 'unpause' taken care of in here
  print_job_timer.stop();
  quickstop_stepper();
  print_job_timer.stop();
  BUZZ(200, 100);
   // Wait a short time (allows messages to get out before shutting down.
  for (int i = 1000; i--;) DELAY_US(600); //just turning off interrupt causes an immediate stop but also a reset.
  cli(); // Stop interrupts - IT WILL REBOOT SOON
}


void menu_tension() {
  char myStr [50] = "";
  static int cur_speed = int(wind_speed);
  //char myStr1 [20] = "";
  extern const char G1_WINDER[];  // PGMSTR(G1_WINDER,"M810 G1 X%d\nM811 G1 X%d"));
  int filament_Length = ((Hit_Pos/2) * 10);
  START_MENU();
  BACK_ITEM(MSG_REWINDER);    
  ui.defer_status_screen();
  if (!allow_job) {
    sprintf_P(myStr, G1_WINDER, Hit_Pos, filament_Length, 1, filament_Length, 1500);  //send the M810 and M811 commands to set their functions
    SERIAL_ECHOLN("WINDER..Setting M810, M811, M812 commands");
    SERIAL_ECHOLN(myStr);                                                       //in this case M810 - G1 Xxx Eyyy and M811 G1 X1 Eyyy
    queue.inject(myStr);
    allow_job = true;
  } 
  GCODES_ITEM(MSG_REWINDER_1SLOW, PSTR("G91\nM83\nG1 X1.5 E15 F50\nG90\nM83"));   //
  GCODES_ITEM(MSG_REWINDER_5REGULAR, PSTR("G91\nM83\nG1 X5 E50 F100\nG90\nM83"));   //
  GCODES_ITEM(MSG_REWINDER_2FAST, PSTR("G91\nM83\nG1 X2 E20 F300\nG90\nM83"));   //
  EDIT_ITEM_FAST(float5_25, MSG_REWINDER_SPEED, &wind_speed, 200,REWINDER_MAX_SPEED);
  if (int(wind_speed) != cur_speed) {
    cur_speed = int(wind_speed);
    sprintf_P(myStr, G1_WINDER_SPEED, cur_speed);  //send the M812 command to set speed
    SERIAL_ECHO("WINDER..Setting speed (M812): ");
    SERIAL_ECHOLN(cur_speed);
    SERIAL_ECHOLN(myStr);                          //in this case M812 - G1 Fyyy
    queue.inject(myStr);
  }
  GCODES_ITEM(MSG_REWINDER_RELEASE, PSTR("M18"));   //Disable Extruder stepper motor
END_MENU();
}



#if ENABLED(REWINDER)

  void menu_rewinder() {
    char chr1 = ' ', chr2 = ' ', chr3 = ' ', chr4 = ' ';
    static bool quiet = false;
    ui.defer_status_screen();
    START_MENU();
    BACK_ITEM(MSG_MAIN);
    //SERIAL_ECHOLN("WINDER..In Winder menu");
    ACTION_ITEM(MSG_REWINDER_PROBE,Probe_Spool);
    if (Hit_Pos != X_BED_SIZE) {
      //SERIAL_ECHOLN("WINDER..Spool metrics acquired");
      SUBMENU(MSG_REWINDER_TENSION, menu_tension);
      //EDIT_ITEM(uint16_4, MSG_REWINDER_SPEED, &ttt, 0, 100);
     if (allow_job) {
        //SERIAL_ECHOLN("WINDER..Prerequesites satisfied");
        GCODES_ITEM(MSG_REWINDER_TEST, PSTR("G90\nM83\nG1 F200\nM810\nM812\nM811\nM810\nM811\nM810\nM811"));  
        GCODES_ITEM(MSG_REWINDER_REWIND, PSTR("G90\nM83\nG1 F200\nM810\nM812\nM811\nM810\nM811\nM810\nM811\nM810\nM811\nM810\nM811\nM810\nM811\nM810\nM811\nM810\nM811\nM810\nM811\nM810\nM811\nM810\nM811\nM810\nM811\nM810\nM811\nM810\nM811\nM810\nM811\nM810\nM811\nM810\nM811\nM810\nM811\nM810\nM811\nM810\nM811\nM810\nM811\nM810\nM811\nM810\nM811\nM810\nM811\nM810\nM811\nM810\nM811\nM810\nM811\nM810\nM811\nM810\nM811\nM810\nM811\nG1 X10"));
        if  (printer_busy()) {
          if (!quiet) {
            SERIAL_ECHOLN("WINDER..Winding now..");
            quiet = true;
          }
          TERN_(HAS_WIRED_LCD, ui.status_printf_P(0, PSTR(S_FMT " %c %c %c %c"), GET_TEXT(MSG_REWINDER_STATUS), chr1, chr2, chr3, chr4));
          ACTION_ITEM(MSG_REWINDER_KILL,Kill_Spool);
        }
        else {
          if (quiet) {
            SERIAL_ECHOLN("WINDER..Completed..");
            quiet = false;
          }
          TERN_(HAS_WIRED_LCD, ui.status_printf_P(0, PSTR(S_FMT " %c %c %c %c"), GET_TEXT(MSG_REWINDER_FINISHED), chr1, chr2, chr3, chr4));
        }
      }
    } 
    if (READ(19) && (printer_busy())) {    //rewinder
      SERIAL_ECHOLN("WINDER..We ranout of filament!");
    };
    if (need_filament_first) {
      STATIC_ITEM(MSG_REWINDER_BLANK);
      STATIC_ITEM(MSG_REWINDER_BLANK);
      STATIC_ITEM(MSG_REWINDER_FILAMENT);
    }
    END_MENU();
  }
#endif


#endif // HAS_LCD_MENU
