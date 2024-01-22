#ifndef __GRID_LIST_BOX_H___
#define __GRID_LIST_BOX_H___

#include "componentcanvas.hpp"
#include "tftmanager.hpp"

class GridListBox: public ComponentCanvas {
public:
  GridListBox(CanvasState& canvasState);

  virtual ~GridListBox() {
    FreeResources();
  }

  void FreeResources();

  // Overridden Component methods
  virtual void Draw();
  virtual void Redraw();

  void Init(uint8_t _columns, uint8_t _rows);
  void SetWidthAndHeight(uint16_t _width, uint16_t _height);

  // Column numbers start at zero, like any sane numbering system.
  // Width is a percentage. It's the caller's responsibility to ensure
  // all column widths add up to 100, otherwise bad things will happen.
  bool AllocColumn(uint8_t colNum, uint16_t size, uint16_t percentWidth);

  // Must be called before calling SetData on a row.
  // Returns the index of the new row, or 0xff if a new row could not be
  // inserted (max rows reached). 
  uint8_t InsertRow();

  // Caller's responsibility to ensure data will fit into the buffer.
  void SetData(uint8_t colNum, uint8_t rowNum, const char* data);
  void ClearValidRows() { validRows = 0; }
  uint8_t GetValidRows() { return validRows; }

  // Return a pointer to the data in the specified cell.
  const char* GetCellData(uint8_t colNum, uint8_t rowNum);

  void SetSelectedTextColor(uint16_t _textColor, uint16_t _textBgColor = TFT_LIGHTGREY);

  // Each of these routines return true if the next / prev item was selected, false
  // otherwise (beginning or end of list).
  bool SelectNextItem();
  bool SelectPrevItem();
  void SelectFirstItem() { selection = 0; }
  void SelectLastItem() { selection = validRows - 1; }

  const char* GetSelectedCellData(uint8_t colNum);
  uint8_t GetSelectionIndex() { return selection; }

private: 
  uint16_t getLineHeight();
  void fillRow(uint8_t rowNum, uint16_t fillColor);
  void selectionChanged(uint8_t prevSelection);
  void printRow(uint8_t row, bool erase = true);

  uint8_t   selection;
  uint16_t  selectionTextColor;
  uint16_t  selectionBgTextColor;

  uint16_t width, height;

  uint8_t columns;
  uint8_t rows;
  uint8_t validRows;
  
  char ***columnData;
  uint16_t *columnMaxSizes;
  uint16_t *columnWidthData;
  uint16_t maxRowSize;
};

#endif
