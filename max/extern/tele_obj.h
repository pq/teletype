#ifndef _TELE_OBJ_H_
#define _TELE_OBJ_H_

///////////////////////////////////////////////////////////////////////////////
// tonnetz
const char STARTER_SCRIPT[] =
    "#I:\n"
    "\n"
    "X  9\n"
    "Y 12\n"
    "Z 16\n"
    "T 0\n"
    "L 1 3 : TR.TOG I\n"
    "SCRIPT 8\n"
    "\n"
    "#1\n"
    "\n"
    "IF EZ T : Y ADD X 3\n"
    "ELSE : Y ADD X 4\n"
    "Z ADD X 7\n"
    "SCRIPT 8\n"
    "\n"
    "#2\n"
    "\n"
    "IF EZ T : X ADD X 4\n"
    "ELSE : X SUB X 4\n"
    "IF EZ T : Y ADD X 3\n"
    "ELSE : Y ADD X 4\n"
    "Z ADD X 7\n"
    "SCRIPT 8\n"
    "\n"
    "#3\n"
    "\n"
    "IF EZ T : X SUB X 3\n"
    "ELSE : X ADD X 3\n"
    "IF EZ T : Y ADD X 3\n"
    "ELSE : Y ADD X 4\n"
    "Z ADD X 7\n"
    "SCRIPT 8\n"
    "\n"
    "#4\n"
    "SCRIPT I\n"
    "\n"
    "#5\n"
    "X ADD X 1\n"
    "Y ADD Y 1\n"
    "Z ADD Z 1\n"
    "SCRIPT 8\n"
    "\n"
    "#8\n"
    "\n"
    "CV 1 N X\n"
    "CV 2 N Y\n"
    "CV 3 N Z\n"
    "T FLIP\n"
    "\n";


#endif  //_TELE_OBJ_H_
