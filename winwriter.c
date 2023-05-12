
/*** INCLUDES : ***/

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <strsafe.h>

/*** DEFINES : ***/

#define CTRL_KEY(k) ((k) & 0x1f)

#ifndef ENABLE_VIRTUAL_TERMINAL_INPUT
#define ENABLE_VIRTUAL_TERMINAL_INPUT 0x0200
#endif

#define WHITE_FONT FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE

/*** DATA : ***/

/* Structure qui stocke les donnes de configuration de la console */
struct USER_CONSOLE_CONFIG
{
    HANDLE hConsoleInput;
    HANDLE hConsoleOutput;

    DWORD originalConsoleInputMode;
    DWORD originalConsoleOutputMode;
};

PCONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo;

struct USER_CONSOLE_CONFIG U = {0};;

/* Structure qui stocke le buffer de la console, toutes les fonctions qui modifient ce que la console affichent modifient
   d'abord le buffer. Le buffer n'est affiché qu'une fois par refresh. */
typedef struct
{
    HANDLE hConsoleBuffer;
    COORD size;
    CHAR_INFO* data;
}CONSOLE_BUFFER;

CONSOLE_BUFFER bConsoleBuffer = { 0 };

//INIT
void InitializeConsole();

//TERMINAL
void ErrorExit(LPTSTR lpszFunction);
void EnableRawMode();
void DisableRawMode();
void HandleKeyEvent();

//INPUT
char ReadInput();

//OUTPUT
void UpdateConsoleScreen();
void ClearConsoleScreen();
void CursorToOrigin();

//BUFFER
void InitConsoleBuffer();
void WriteConsoleBuffer(CONSOLE_BUFFER* buffer, char c, WORD attributes);
void ClearConsoleBuffer(CONSOLE_BUFFER* buffer, WORD attributes);
void DisplayConsoleBuffer(CONSOLE_BUFFER* buffer);


/*** INIT : ***/

void InitializeConsole()
{
    // Recupere les handles de buffer d'entree et sortie standards
    U.hConsoleInput = GetStdHandle(STD_INPUT_HANDLE);
    if(U.hConsoleInput == INVALID_HANDLE_VALUE)
    {
        ErrorExit(TEXT("GetStdHandle"));
    }

    U.hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    if(U.hConsoleOutput == INVALID_HANDLE_VALUE)
    {
        ErrorExit(TEXT("GetStdHandle"));
    }

    // Recupere le pointeur vers le buffer d'affichage de la console
    if(!GetConsoleScreenBufferInfo(U.hConsoleOutput, &lpConsoleScreenBufferInfo))
    {
        ErrorExit(TEXT("GetConsoleScreenBufferInfo"));
    }

    // Recupere le mode actuelle de la console
    if(!GetConsoleMode(U.hConsoleInput, &U.originalConsoleInputMode))
    {
        ErrorExit(TEXT("GetConsoleMode"));
    }

    // Passe la console en mode "Raw"
    EnableRawMode();

    // Initialiser le buffer de la console
    InitConsoleBuffer();

    //Lorsque le programme s'arrete on remet la console dans son mode d'origine
    atexit(DisableRawMode);
}

int main()
{
    InitializeConsole();

    while(1)
    {
        UpdateConsoleScreen();

        HandleKeyEvent();
    }


    // Libérer la mémoire et fermer le handle du buffer de console
    free(bConsoleBuffer.data);
    CloseHandle(bConsoleBuffer.hConsoleBuffer);

    return(0);
}

/*** TERMINAL : ***/

void EnableRawMode()
{
    DWORD currentInputMode = U.originalConsoleInputMode;

    currentInputMode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_EXTENDED_FLAGS);
    currentInputMode |= ENABLE_VIRTUAL_TERMINAL_INPUT ;
    if (!SetConsoleMode(U.hConsoleInput, currentInputMode))
    {
        ErrorExit(TEXT("SetConsoleMode"));
    }
}

void DisableRawMode()
{
    if (!SetConsoleMode(U.hConsoleInput, U.originalConsoleInputMode))
    {
        ErrorExit(TEXT("SetConsoleMode"));
    }
}

void ErrorExit(LPTSTR lpszFunction)
{
    // Retrieve the system error message for the last-error code

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError();

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    // Display the error message and exit the process

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
                                      (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
    StringCchPrintf((LPTSTR)lpDisplayBuf,
                    LocalSize(lpDisplayBuf) / sizeof(TCHAR),
                    TEXT("%s failed with error %d: %s"),
                    lpszFunction, dw, lpMsgBuf);
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    ExitProcess(dw);
}

void HandleKeyEvent()
{
    char c = ReadInput();

//    if (iscntrl(c))
//    {
//        printf("%d\n", c);
//    }
//    else
//    {
//        printf("%d ('%c')\n", c, c);
//    }

    switch (c)
    {
    case CTRL_KEY('q'):
    {
        //editorClearScreen(&ab);
        //editorSetCursorOrigin(&ab);
        exit(0);
        break;
    }
    default:
    {
        WriteConsoleBuffer(&bConsoleBuffer, c, WHITE_FONT);
    }
    }
}


/*** INPUT : ***/

char ReadInput()
{
    INPUT_RECORD inputRecord;
    DWORD nRead;

    while (1)
    {
        if (!ReadConsoleInput(U.hConsoleInput, &inputRecord, 1, &nRead))
        {
            ErrorExit(TEXT("ReadConsoleInput"));
        }
        if (inputRecord.EventType == KEY_EVENT && inputRecord.Event.KeyEvent.bKeyDown)
        {
            return inputRecord.Event.KeyEvent.uChar.AsciiChar;
        }
    }
}

/*** OUTPUT : ***/

void UpdateConsoleScreen()
{
    ClearConsoleScreen();
    CursorToOrigin();

    DisplayConsoleBuffer(&bConsoleBuffer);

    CursorToOrigin();
}

void ClearConsoleScreen()
{
    PCWSTR sequence = "\x1b[2J";
    DWORD written = 0;

    WriteConsole(U.hConsoleOutput, sequence, (DWORD)wcslen(sequence), &written, NULL);
}

void CursorToOrigin()
{
    PCWSTR sequence = "\x1b[0;0H";
    DWORD written = 0;

    WriteConsole(U.hConsoleOutput, sequence, (DWORD)wcslen(sequence), &written, NULL);
}



/*** BUFFER : ***/

// Initialiser le buffer
void InitConsoleBuffer()
{
    // Creation du handle vers le buffer
    bConsoleBuffer.hConsoleBuffer = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    if(bConsoleBuffer.hConsoleBuffer == INVALID_HANDLE_VALUE)
    {
        ErrorExit(TEXT("CreateConsoleScreenBuffer"));
    }

    // La taille du buffer est la taille de la fenetre
    //bConsoleBuffer.size = U.lpConsoleScreenBufferInfo->dwSize;
    bConsoleBuffer.size = (COORD){80, 25};
    bConsoleBuffer.data = (CHAR_INFO*)malloc(bConsoleBuffer.size.X * bConsoleBuffer.size.Y * sizeof(CHAR_INFO));
    if(bConsoleBuffer.data == NULL)
    {
        ErrorExit(TEXT("malloc"));
    }
}


// Écrire dans le buffer
void WriteConsoleBuffer(CONSOLE_BUFFER* buffer, char c, WORD attributes)
{
    int x = lpConsoleScreenBufferInfo->dwCursorPosition.X;
    int y = lpConsoleScreenBufferInfo->dwCursorPosition.Y;

    if (x >= 0 && x < buffer->size.X && y >= 0 && y < buffer->size.Y)
    {
        buffer->data[y * buffer->size.X + x].Char.AsciiChar = c;
        buffer->data[y * buffer->size.X + x].Attributes = attributes;
    }
}

// Vider le buffer
void ClearConsoleBuffer(CONSOLE_BUFFER* buffer, WORD attributes)
{
    for (int y = 0; y < buffer->size.Y; y++)
    {
        for (int x = 0; x < buffer->size.X; x++)
        {
            WriteConsoleBuffer(buffer, ' ', attributes);
        }
    }
}

// Afficher le contenu du buffer dans la console
void DisplayConsoleBuffer(CONSOLE_BUFFER* ConsoleBuffer)
{
    SMALL_RECT region = {0, 0, ConsoleBuffer->size.X - 1, ConsoleBuffer->size.Y - 1};

    if(!SetConsoleActiveScreenBuffer(ConsoleBuffer->hConsoleBuffer))
    {
        ErrorExit(TEXT("SetConsoleActiveScreenBuffer"));
    }

    if(!WriteConsoleOutput(ConsoleBuffer->hConsoleBuffer, ConsoleBuffer->data, ConsoleBuffer->size, (COORD)
{
    0, 0
}, &region))
    {
        ErrorExit(TEXT("WriteConsoleOutput"));
    }
}

