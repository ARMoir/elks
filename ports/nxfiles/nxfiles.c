/*
 * nxfiles: read-only, tiny graphical file browser for ELKS Nano-X.
 * Single static window; no X11, Motif, dynamic allocation, or writes.
 * The text preview is intentionally plain and limited to protect the
 * tiny 8086/286 memory footprint and floppy-based filesystem.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include "nano-X.h"
#include "nxcolors.h"

#define W 414
#define H 295
#define ROWS 11
#define RH 17
#define ROW_Y 51
#define MAX_FILES 72
#define NAME_LEN 40
#define PATH_LEN 128
#define PAGE_STEP ROWS

static GR_WINDOW_ID win;
static GR_GC_ID gc;
static char cwd[PATH_LEN] = "/";
static char preview[PATH_LEN];
static char names[MAX_FILES][NAME_LEN];
static unsigned char folders[MAX_FILES];
static int nfiles, offset, page, viewing, more_files;
static int screen_ready;
static char status[64];

static void activate_file_window(void)
{
    GR_WINDOW_INFO info;
    GrGetWindowInfo(win, &info);
    if (info.wid != win)
        return;
    GrRaiseWindow(info.parent && info.parent != GR_ROOT_WINDOW_ID
                  ? info.parent : win);
    GrSetFocus(win);
}

static void rect(int x, int y, int w, int h, GR_COLOR c)
{
    GrSetGCForeground(gc, c);
    GrFillRect(win, gc, x, y, w, h);
}
static void line(int x1, int y1, int x2, int y2, GR_COLOR c)
{
    GrSetGCForeground(gc, c);
    GrLine(win, gc, x1, y1, x2, y2);
}
static void text(int x, int y, const char *s, GR_COLOR c)
{
    GrSetGCForeground(gc, c);
    GrText(win, gc, x, y, (void *)s, strlen(s), GR_TFASCII);
}
static void bevel(int x, int y, int w, int h)
{
    line(x, y, x+w-1, y, GR_COLOR_WHITE);
    line(x, y, x, y+h-1, GR_COLOR_WHITE);
    line(x+w-1, y, x+w-1, y+h-1, GR_COLOR_DIMGRAY);
    line(x, y+h-1, x+w-1, y+h-1, GR_COLOR_DIMGRAY);
}
static void button(int x, const char *caption)
{
    rect(x, 262, 87, 24, GR_COLOR_GRAY);
    bevel(x, 262, 87, 24);
    text(x+18, 279, caption, GR_COLOR_BLACK);
}
/* Single drive/root filesystem and no hidden system state. */
static int make_path(char *dest, const char *name)
{
    int a = strlen(cwd);
    int b = strlen(name);
    if (a+b+2 >= PATH_LEN)
        return -1;
    strcpy(dest, cwd);
    if (a > 1)
        strcat(dest, "/");
    strcat(dest, name);
    return 0;
}
static void parent_directory(void)
{
    char *slash;
    if (!strcmp(cwd, "/"))
        return;
    slash = strrchr(cwd, '/');
    if (!slash || slash == cwd)
        strcpy(cwd, "/");
    else
        *slash = '\0';
}

static void scan_directory(void)
{
    DIR *dir;
    struct dirent *ent;
    struct stat st;
    char full[PATH_LEN];

    nfiles = 0;
    offset = 0;
    more_files = 0;
    strcpy(status, "Select folder or text file");
    dir = opendir(cwd);
    if (!dir) {
        strcpy(status, "Unable to read directory");
        return;
    }
    while ((ent = readdir(dir)) != NULL) {
        if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
            continue;
        if (nfiles == MAX_FILES) {
            more_files = 1;
            break;
        }
        if (strlen(ent->d_name) >= NAME_LEN)
            continue;
        strcpy(names[nfiles], ent->d_name);
        folders[nfiles] = 0;
        if (make_path(full, ent->d_name) == 0 && stat(full, &st) == 0)
            folders[nfiles] = S_ISDIR(st.st_mode) ? 1 : 0;
        ++nfiles;
    }
    closedir(dir);
    if (more_files)
        strcpy(status, "First 72 entries only");
}

/* One text line per screen row. Reading stops at the requested page.
 * Do not attempt to interpret executable or binary files. */
static void draw_preview(void)
{
    FILE *fp;
    char buf[64];
    int skip = page * ROWS;
    int i = 0;
    int j;

    fp = fopen(preview, "r");
    if (!fp) {
        text(13, 70, "Cannot open file for reading.", GR_COLOR_BLACK);
        return;
    }
    while (skip-- > 0 && fgets(buf, sizeof(buf), fp))
        ;
    while (i < ROWS && fgets(buf, sizeof(buf), fp)) {
        for (j=0; buf[j]; ++j) {
            if ((unsigned char)buf[j] == '\r' ||
                (unsigned char)buf[j] == '\n') {
                buf[j] = '\0';
                break;
            }
            if (((unsigned char)buf[j] < 32 &&
                buf[j] != '\t') || (unsigned char)buf[j] >= 127) {
                fclose(fp);
                rect(10, ROW_Y, W-20, ROWS*RH+2, GR_COLOR_WHITE);
                text(13, 70, "Non-text content; preview unavailable.", GR_COLOR_BLACK);
                return;
            }
        }
        if (j > 50) buf[50] = '\0';
        text(15, 66 + RH*i, buf, GR_COLOR_BLACK);
        ++i;
    }
    fclose(fp);
    if (!i)
        text(13, 70, "[End of file]", GR_COLOR_BLACK);
}

static void redraw(void)
{
    int i, row, y;
    char b[48];

    rect(0, 0, W, H, GR_COLOR_GRAY);
    bevel(0, 0, W, H);
    rect(4, 3, W-8, 21, GR_COLOR_DARKSLATEGRAY);
    text(11, 18, viewing ? "FILE VIEWER (read only)" :
         "ELKS FILE MANAGER", GR_COLOR_WHITE);
    rect(8, 28, W-16, 20, GR_COLOR_LIGHTSTEELBLUE);
    if (viewing) {
        const char *name = strrchr(preview, '/');
        text(14, 43, name ? name+1 : preview, GR_COLOR_BLACK);
    } else
        text(14, 43, cwd, GR_COLOR_BLACK);
    rect(10, ROW_Y, W-20, ROWS*RH+2, GR_COLOR_WHITE);

    if (viewing) {
        draw_preview();
    } else {
        for (row=0; row<ROWS; ++row) {
            i = offset+row;
            if (i >= nfiles) break;
            y = ROW_Y + row*RH;
            if (folders[i]) {
                rect(16, y+4, 13, 9, GR_COLOR_NAVY);
                rect(17, y+5, 11, 7, GR_COLOR_LIGHTSTEELBLUE);
            } else {
                rect(18, y+3, 9, 12, GR_COLOR_DARKGRAY);
                rect(20, y+5, 5, 8, GR_COLOR_WHITE);
            }
            strncpy(b, names[i], 47);
            b[47] = '\0';
            text(40, y+13, b, GR_COLOR_BLACK);
            line(12, y+RH-1, W-13, y+RH-1, GR_COLOR_LIGHTGRAY);
        }
    }
    text(11, 253, viewing ? "Read-only preview" : status, GR_COLOR_BLACK);
    button(9, viewing ? "Back" : "Up");
    button(110, "Prev");
    button(211, "Next");
    button(312, "Close");
    screen_ready = 1;
}

static void select_entry(int index)
{
    char full[PATH_LEN];

    if (index < 0 || index >= nfiles)
        return;
    if (make_path(full, names[index]) != 0) {
        strcpy(status, "Path too long");
        redraw();
        return;
    }
    if (folders[index]) {
        strcpy(cwd, full);
        scan_directory();
    } else {
        strcpy(preview, full);
        viewing = 1;
        page = 0;
    }
    redraw();
}

int main(void)
{
    GR_EVENT e;
    int row;

    if (GrOpen() < 0)
        return 1;
    win = GrNewWindowEx(GR_WM_PROPS_APPWINDOW | GR_WM_PROPS_NORESIZE, "Files",
                        GR_ROOT_WINDOW_ID, 90, 55, W, H, GR_COLOR_GRAY);
    gc = GrNewGC();
    GrSetGCUseBackground(gc, GR_FALSE);
    GrSetGCFont(gc, GrCreateFontEx(GR_FONT_SYSTEM_FIXED, 0, 0, NULL));
    GrSelectEvents(win, GR_EVENT_MASK_EXPOSURE |
                        GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_CLOSE_REQ);
    scan_directory();
    GrMapWindow(win);

    for (;;) {
        GrGetNextEvent(&e);
        if (e.type == GR_EVENT_TYPE_EXPOSURE) {
            if (e.exposure.wid == win)
                redraw();
        } else if (e.type == GR_EVENT_TYPE_CLOSE_REQ) {
            break;
        } else if (e.type == GR_EVENT_TYPE_BUTTON_DOWN &&
                   e.button.wid == win && screen_ready) {
            activate_file_window();
            if (e.button.y >= ROW_Y &&
                e.button.y < ROW_Y+ROWS*RH && !viewing) {
                row = (e.button.y-ROW_Y)/RH;
                select_entry(offset+row);
            } else if (e.button.y >= 262 && e.button.y < 287) {
                if (e.button.x >= 312) {
                    break;
                } else if (e.button.x >= 211) {
                    if (viewing)
                        ++page;
                    else if (offset+ROWS<nfiles)
                        offset+=ROWS;
                    redraw();
                } else if (e.button.x >= 110) {
                    if (viewing && page>0) --page;
                    else if (!viewing && offset>=ROWS) offset-=ROWS;
                    redraw();
                } else if (e.button.x >= 9) {
                    if (viewing) {
                        viewing = 0;
                    } else {
                        parent_directory();
                        scan_directory();
                    }
                    redraw();
                }
            }
        }
    }
    GrDestroyGC(gc);
    GrDestroyWindow(win);
    GrClose();
    return 0;
}

