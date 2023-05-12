NAMING CONVENTION :

    -   variables       : nomVariable
    -   fonctions       : NomFonction
    -   structs & enums : NOM_STRUCTURE / NOMENUM

SCHEMA DU PROGRAMME :

main()
│
├── InitializeConsole()
│   ├── GetStdHandle()
│   ├── GetConsoleScreenBufferInfo()
│   ├── GetConsoleMode()
│   ├── EnableRawMode()
│   └── DisableRawMode()
│
└── while(1)
    ├── ReadInput()
    │   └── ReadConsoleInput()
    │
    ├── HandleKeyEvent()
    │   ├── HandleArrowKeyEvent()
    │   │   ├── move_cursor_up()
    │   │   ├── move_cursor_down()
    │   │   ├── move_cursor_left()
    │   │   └── move_cursor_right()
    │   │
    │   ├── HandleCharacterInput()  // Insert ou efface des caracteres dans le buffer
    │   │   ├── insert_character()
    │   │   └── delete_character()
    │   │
    │   └── HandleSpecialKeys()
    │       ├── handle_backspace()
    │       ├── handle_delete()
    │       └── handle_enter()
    │
    └── update_console_display()    // Affiche le buffer dans la console
        └── WriteConsoleOutput()



https://learn.microsoft.com/en-us/windows/console/createconsolescreenbuffer

https://learn.microsoft.com/en-us/windows/console/console-screen-buffers

https://learn.microsoft.com/en-us/windows/console/console-screen-buffer-info-str

https://viewsourcecode.org/snaptoken/kilo/03.rawInputAndOutput.html

https://www.lri.fr/~pa/FLWiX/index.php

https://www.finseth.com/craft/craft.pdf
