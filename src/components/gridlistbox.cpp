#include <memory.h>
#include <stdlib.h>

#include "components/canvasstate.hpp"
#include "components/gridlistbox.hpp"
#include "log.hpp"
#include "tftmanager.hpp"


// Datum ML_DATUM apparently doesn't REALLY mean the middle Y value, at least with
// certain fonts with TFT_eSPI. Fonts are getting drawm above the midline. This
// constant is an attempt to compute the actual midpoint.
#define DATUM_FUDGE 0.80


GridListBox::GridListBox(CanvasState& canvasState):
  ComponentCanvas(canvasState),
  selection(0),
  selectionTextColor(TFT_BLACK),
  selectionBgTextColor(TFT_LIGHTGREY),
  width(0),
  height(0),
  columns(1),
  rows(1),
  validRows(0),
  columnData(NULL),
  columnMaxSizes(0),
  columnWidthData(NULL),
  maxRowSize(0) {
    ourCanvasState.datum = ML_DATUM; // Start from the middle y value of each row
  }


void GridListBox::FreeResources() {
  logPrintf(LOG_COMP_GRID, LOG_SEV_VERBOSE, "Freeing gridlist resources\n");
  if (columnData) {
    for (uint8_t i = 0; i < columns; i++) {
      logPrintf(LOG_COMP_GRID, LOG_SEV_DEBUG, "Freeing row %d\n", i);
      char **freeThis = columnData[i];
      if (freeThis) {
        for (uint8_t j = 0; j < columns; j++) {
          char *freeColumn = freeThis[j];
          logPrintf(LOG_COMP_GRID, LOG_SEV_DEBUG, "Freeing column %d, contents: %s\n", j, freeColumn);
          free(freeColumn);
        }

        free(freeThis);
      }      
    }

    free(columnData);
  }

  if (columnWidthData) {
    free(columnWidthData);
  }

  if (columnMaxSizes) {
    free(columnMaxSizes);
  }
}


void GridListBox::Init(uint8_t _columns, uint8_t _rows) {
  columns = _columns;
  rows = _rows;

  columnData = (char***)malloc(columns * sizeof(char*));
  if (!columnData) {
    Serial.println("**** OUT OF MEMORY allocating columns");
    return;
  }
  
  memset(columnData, 0, columns * sizeof(char*));

  columnWidthData = (uint16_t*)malloc(columns * sizeof(uint16_t));
  columnMaxSizes = (uint16_t*)malloc(columns * sizeof(uint16_t));
}


void GridListBox::SetWidthAndHeight(uint16_t _width, uint16_t _height) {
  width = _width;
  height = _height;
}


// Column numbers start at zero, like any sane numbering system.
// Width is a percentage. It's the caller's responsibility to ensure
// all column widths add up to 100, otherwise bad things will happen.
bool GridListBox::AllocColumn(uint8_t colNum, uint16_t size, uint16_t percentWidth) {
  char **theColumn = (char**)malloc(rows * sizeof(char*));
  if (!theColumn) {
    Serial.print("**** OUT OF MEMORY allocating column "); Serial.println(colNum);
    return false;
  }

  memset(theColumn, 0, rows * sizeof(char*));

  for (uint8_t i = 0; i < rows; i++) {
    theColumn[i] = (char*)malloc(size + 1); // Leave room for the NULL
    if (!theColumn[i]) {
      Serial.print("**** OUT OF MEMORY allocating rows for column "); Serial.println(colNum);
      return false;
    }

    // Start with empty columns and ensure the buffer ends in a NULL
    // terminator for strncpy.
    theColumn[i][0] = theColumn[i][size] = '\0';
  }

  columnData[colNum] = theColumn;
  columnMaxSizes[colNum] = size;
  columnWidthData[colNum] = percentWidth;

  return true;
}


uint8_t GridListBox::InsertRow() {
  if (validRows < rows) {
    return validRows++;
  } else {
    return -1;
  }
}


// Caller's responsibility to ensure data will fit into the buffer.
void GridListBox::SetData(uint8_t colNum, uint8_t rowNum, const char* data) {
  strncpy(columnData[colNum][rowNum], data, columnMaxSizes[colNum]);
}


const char* GridListBox::GetCellData(uint8_t colNum, uint8_t rowNum) {
  const char* data = NULL;

  if (colNum <= columns && rowNum <= validRows) {
    data = columnData[colNum][rowNum];
  }

  return data;
}


void GridListBox::SetSelectedTextColor(uint16_t _textColor, uint16_t _textBgColor) {
  selectionTextColor = _textColor;
  selectionBgTextColor = _textBgColor;
}


uint16_t GridListBox::getLineHeight() {
  return height / rows;
}


void GridListBox::fillRow(uint8_t rowNum, uint16_t fillColor) {
  uint16_t lineHeight = getLineHeight();
  uint16_t top = ourCanvasState.cursorY + rowNum * lineHeight;
  logPrintf(
    LOG_COMP_GRID, 
    LOG_SEV_DEBUG,
    "fillRow: x: %d, top: %d, width: %d, height: %d, addr: %d\n",
    ourCanvasState.cursorX, top, width, getLineHeight(), TftManager::GetTft());

  TftManager::GetTft()->fillRect(ourCanvasState.cursorX, top, width, getLineHeight(), fillColor);
}


bool GridListBox::SelectNextItem() {
  logPrintf(LOG_COMP_SONG, LOG_SEV_DEBUG, "SelectNextItem: selection: %d, validRows: %d\n", 
    selection, validRows);

  if (selection + 1 >= validRows) {
    return false;
  }

  uint8_t prevSelection = selection++;
  selectionChanged(prevSelection);

  return true;
}


bool GridListBox::SelectPrevItem() {
  if (selection == 0) {
    return false;
  }

  uint8_t prevSelection = selection--;
  selectionChanged(prevSelection);
  return true;
}


void GridListBox::selectionChanged(uint8_t prevSelection) {
  CanvasState canvasState;
  ourCanvasState.Apply();

  printRow(prevSelection);
  printRow(selection);
}


const char* GridListBox::GetSelectedCellData(uint8_t colNum) {
  const char* data = NULL;

  if (colNum < columns) {
    data = columnData[colNum][selection];
  }

  return data;
}


void GridListBox::printRow(uint8_t row, bool changeFont) {
  TFT_eSPI *tft = TftManager::GetTft();

  uint16_t bgColor = TFT_BLACK;
  uint16_t fgColor = TFT_LIGHTGREY;
  if (row == selection) {
    bgColor = selectionBgTextColor;
    fgColor = selectionTextColor;
  } else {
    fgColor = ourCanvasState.fgColor;
    bgColor = ourCanvasState.bgColor;
  }

  logPrintf(LOG_COMP_GRID, LOG_SEV_VERBOSE, "Grid: fgColor: %d, bgColor: %d\n", fgColor, bgColor);
  tft->setTextColor(fgColor, bgColor);
  fillRow(row, bgColor);

  uint16_t lineHeight = getLineHeight();

  // Using ML_DATUM, we need to specify the middle y value for the text. That
  // way the text always appears in the middle-left of the cell and we don't
  // have to compute the y value for it.
  uint16_t cursorY = ourCanvasState.cursorY + lineHeight * row + lineHeight * DATUM_FUDGE;

  logPrintf(LOG_COMP_GRID, LOG_SEV_VERBOSE, "y: %d, lineHeight: %d, row: %d, cursorY: %d\n", 
    ourCanvasState.cursorY, lineHeight, row, cursorY);

  uint16_t cursorX = ourCanvasState.cursorX;
  for (uint8_t j = 0; j < columns; j++) {
    tft->setCursor(cursorX, cursorY);
    char **theColumn = columnData[j];
    
    logPrintf(LOG_COMP_GRID | LOG_COMP_SONG, LOG_SEV_DEBUG, "cursorX: %d, cursorY: %d, string: %s\n", 
      cursorX, cursorY, theColumn[row]);

    tft->print(theColumn[row]);
    cursorX += (((float)columnWidthData[j] / 100) * width);
  }
}


void GridListBox::Redraw() {
  CanvasState canvasState;
  ourCanvasState.Apply();

  TFT_eSPI *tft = TftManager::GetTft();
  tft->fillRect(ourCanvasState.cursorX, ourCanvasState.cursorY, width, height, ourCanvasState.bgColor);
  Draw();
}


void GridListBox::Draw() {
  CanvasState canvasState;
  ourCanvasState.Apply();

  for (uint8_t i = 0; i < validRows; i++) {
    // If we're here, then we're drawing on a fresh canvas
    printRow(i, false); 
  }
}
