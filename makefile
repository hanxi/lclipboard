SRC_DIR=.
OBJ_DIR=objs


INC=/I $(SRC_DIR) /I $(LUA_INSTALL_PATH)\include
LUA_LIB_PATH=$(LUA_INSTALL_PATH)\lib
LUA_LIB=lua.lib


DEF= /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" \
     /D "_CRT_SECURE_NO_WARNINGS" \
     /D "_WINDLL"  \
     /D LUA_BUILD_AS_DLL \
     /D UNICODE /D _UNICODE

CFLAGS=$(LUA_INC) /c /O2 /Ot /MT /W3 /nologo $(DEF)
LDFLAGS= /NOLOGO /DLL /SUBSYSTEM:windows \
    /LIBPATH:"$(LUA_LIB_PATH)" $(LUA_LIB) \
    user32.lib shell32.lib /OUT:

# /SUBSYSTEM:windows /ENTRY:mainCRTStartup \

INSTALL_CDIR=$(LUA_INSTALL_PATH)\bin

all: clipboard.dll install

#compile
{$(SRC_DIR)}.c{$(OBJ_DIR)}.obj:
    @if not exist $(OBJ_DIR) mkdir $(OBJ_DIR)
    $(CC) $(CFLAGS) $(INC) /Fo$@ $<

# link
clipboard.dll: $(OBJ_DIR)\lclipboard.obj
    LINK /nologo $? $(LDFLAGS)$@

install:
    @if not exist $(INSTALL_CDIR) mkdir $(INSTALL_CDIR)
    @for %I in (clipboard.dll) do copy %I $(INSTALL_CDIR)

clean:
    rd /S /Q $(OBJ_DIR)
    del *.exe
    del *.exp
    del *.lib