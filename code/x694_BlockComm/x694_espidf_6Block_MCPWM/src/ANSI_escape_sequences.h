#pragma once
// ----------------------------------------------------------------------------
//  Operating System Command (OSC) sequences
// ----------------------------------------------------------------------------
#define WIN_TITLE(x) "\x1B]2;" x "\x07" // Set window title to string x

// ----------------------------------------------------------------------------
//  Control Sequence Introducer (CSI) sequences
// ----------------------------------------------------------------------------
#define RST_CUR     "\x1B[1;1H" // Reset cursor position to upper left
// ESC[J	erase in display (same as ESC[0J)
#define CLR_DISP_A  "\x1B[J"   // erase from cursor until end of screen
#define CLR_DISP_B  "\x1B[1J"   // erase from cursor to beginning of scree
#define CLR_DISP    "\x1B[2J"   // erase entire display screen
#define CLR_SAVED  "ESC[3J"	//erase saved lines
// ESC[K	erase in line (same as ESC[0K)
#define CLR_LINE_A  "\x1B[K"   // erase from cursor to end of line
#define CLR_LINE_B  "\x1B[1K"   // erase start of line to the cursor
#define CLR_LINE    "\x1B[2K"   // erase the entire line

//==================================================================
#define CURSOR_HOME  "\x1B[H"	//moves cursor to home position (0, 0)
#define CURSOR(x,y)  "\x1B["#x";"#y"H"	//moves cursor to line #, column #
#define CURSOR_UP(x)  "\x1B["#x"A"	//moves cursor up # lines
#define CURSOR_DOWN(x)  "\x1B["#x"B"	//moves cursor down # lines
#define CURSOR_RIGHT(x)  "\x1B["#x"C"	//moves cursor right # columns
#define CURSOR_LEFT(x)  "\x1B["#x"D"	//moves cursor left # columns
#define CURSOR_START_DOWN(x)  "\x1B["#x"E"	//moves cursor to beginning of next line, # lines down
#define CURSOR_START_UP(x)  "\x1B["#x"F"	//moves cursor to beginning of previous line, # lines up
#define CURSOR_CLM(x)  "\x1B["#x"G"	//moves cursor to column #
#define CURSOR_GET_POS  "\x1B[6n"	//request cursor position (reports as ESC[#;#R)
#define CURSOR_UP1  "\x1B M"	//moves cursor one line up, scrolling if needed
#define CURSOR_SAVE_POS  "\x1B 7"	//save cursor position (DEC)
#define CURSOR_RESTORE  "\x1B 8"	//restores the cursor to the last saved position (DEC)
// #define CURSOR  "\x1B[s"	//save cursor position (SCO)
// #define CURSOR  "\x1B[u"	//restores the cursor to the last saved position (SCO)
#define CURSOR_INVIS "\x1B[?25l"	//make cursor invisible
#define CURSOR_VIS "\x1B[?25h"	//make cursor visible
// ----------------------------------------------------------------------------
//  Select Graphic Rendition (SGR) sequences
// ----------------------------------------------------------------------------
#define RESET "\x1B[0m"  // Reset all SRG parameters

#define BOLD  "\x1B[1m"  // Bold text
#define UND   "\x1B[4m"  // Underline text

// Text colour
//  BLK - Black   BLU - Blue
//  RED - Red     MAG - Magenta
//  GRN - Green   CYN - Cyan
//  YEL - Yellow  WHT - White
//  H-prefix - High intensity variant (brighter)
#define BLK "\x1B[30m"
#define RED "\x1B[31m"
#define GRN "\x1B[32m"
#define YEL "\x1B[33m"
#define BLU "\x1B[34m"
#define MAG "\x1B[35m"
#define CYN "\x1B[36m"
#define WHT "\x1B[37m"
#define HBLK "\x1B[90m"
#define HRED "\x1B[91m"
#define HGRN "\x1B[92m"
#define HYEL "\x1B[93m"
#define HBLU "\x1B[94m"
#define HMAG "\x1B[95m"
#define HCYN "\x1B[96m"
#define HWHT "\x1B[97m"


// Background colour (_B-suffix)
//  BLK - Black   BLU - Blue
//  RED - Red     MAG - Magenta
//  GRN - Green   CYN - Cyan
//  YEL - Yellow  WHT - White
//  H-prefix - High intensity variant (brighter)
#define BLK_B "\x1B[40m"
#define RED_B "\x1B[41m"
#define GRN_B "\x1B[42m"
#define YEL_B "\x1B[43m"
#define BLU_B "\x1B[44m"
#define MAG_B "\x1B[45m"
#define CYN_B "\x1B[46m"
#define WHT_B "\x1B[47m"
#define HBLK_B "\x1B[100m"
#define HRED_B "\x1B[101m"
#define HGRN_B "\x1B[102m"
#define HYEL_B "\x1B[103m"
#define HBLU_B "\x1B[104m"
#define HMAG_B "\x1B[105m"
#define HCYN_B "\x1B[106m"
#define HWHT_B "\x1B[107m"

// 24-bit true colour
#define COL_RGB(r, g, b)    "\x1B[38;2;"#r";"#g";"#b"m" // Select RGB foreground color
#define COL_B_RGB(r, g, b)  "\x1B[48;2;"#r";"#g";"#b"m" // Select RGB background color

#define black "\033[30m"
#define red "\033[31m"
#define green "\033[32m"
#define yellow "\033[33m"
#define blue "\033[34m"
#define magenta "\033[35m"
#define cyan "\033[36m"
#define white "\033[37m"
#define esc "\033[0m"
