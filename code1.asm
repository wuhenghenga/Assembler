FIRST    START   0000
RLOOP    TD      INDEV
         JEQ     RLOOP
         RD      INDEV
         STCH    DATA
OUTLP    TD      OUTDEV
         JEQ     OUTLP
         LDCH    DATA
         WD      OUTDEV
.
INDEV    BYTE    X'05'
OUTDEV   BYTE    X'06'
DATA     RESB    1
         END     FIRST