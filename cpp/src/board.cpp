#include "board.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <iterator>
#include <ranges>

using namespace constants;

// Helper ----------------------------------------------------------------------
template <typename T>
std::ostream& operator<<(std::ostream& os, const Pt<T>& pt)
{
  os << "Pt(x=" << pt.x << ", y=" << pt.y << ")";
  return os;
}

std::ostream& operator<<(std::ostream& os, const Cell& cell)
{
  os << "Cell(contents=" << cell.contents << ", pos=" << cell.pos << ")";
  return os;
}

// Core ------------------------------------------------------------------------
Board::Board()
    : min(START_X + (CELL_WIDTH / 2), START_Y + (CELL_HEIGHT / 2)),
      max(START_X + (BOARD_DIM * CELL_WIDTH) + (CELL_WIDTH / 2),
          START_Y + (BOARD_DIM * CELL_HEIGHT) + (CELL_HEIGHT / 2))
{
  // Initialize the cells of the board.
  for (int i = 0; i < BOARD_DIM; ++i)
  {
    for (int j = 0; j < BOARD_DIM; ++j)
    {
      int index = (i * BOARD_DIM) + j;
      cells[index].contents = EMPTY;
      cells[index].pos.x = START_X + (i * CELL_WIDTH) + (CELL_WIDTH / 2);
      cells[index].pos.y = START_Y + (j * CELL_HEIGHT) + (CELL_HEIGHT / 2);
    }
  }

  // Setup the ncurses GUI.
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, true);

  start_color();
  init_pair(1, COLOR_WHITE, COLOR_BLACK);
  init_pair(2, COLOR_RED, COLOR_BLACK);

  draw();
  center_cursor();
}

Board::~Board()
{
  endwin();
  clear();
}

void Board::draw() const
{
  constexpr int end_y = START_Y + BOARD_DIM * CELL_HEIGHT;
  constexpr int end_x = START_X + BOARD_DIM * CELL_WIDTH;

  for (int j = START_Y; j <= end_y; j += CELL_HEIGHT)
  {
    for (int i = START_X; i <= end_x; ++i)
    {
      mvwaddch(stdscr, j, i, ACS_HLINE);
    }
  }

  for (int i = START_X; i <= end_x; i += CELL_WIDTH)
  {
    for (int j = START_Y; j <= end_y; ++j)
    {
      mvwaddch(stdscr, j, i, ACS_VLINE);
    }
  }

  mvwaddch(stdscr, START_Y, START_X, ACS_ULCORNER);
  mvwaddch(stdscr, end_y, START_X, ACS_LLCORNER);
  mvwaddch(stdscr, START_Y, end_x, ACS_URCORNER);
  mvwaddch(stdscr, end_y, end_x, ACS_LRCORNER);

  for (int j = START_Y + CELL_HEIGHT; j <= end_y - CELL_HEIGHT;
       j += CELL_HEIGHT)
  {
    mvwaddch(stdscr, j, START_X, ACS_LTEE);
    mvwaddch(stdscr, j, end_x, ACS_RTEE);
    for (int i = START_X + CELL_WIDTH; i <= end_x - CELL_WIDTH; i += CELL_WIDTH)
    {
      mvwaddch(stdscr, j, i, ACS_PLUS);
    }
  }

  for (int i = START_X + CELL_WIDTH; i <= end_x - CELL_WIDTH; i += CELL_WIDTH)
  {
    mvwaddch(stdscr, START_Y, i, ACS_TTEE);
    mvwaddch(stdscr, end_y, i, ACS_BTEE);
  }

  // Draw all the played positions
  for (auto& c : cells)
  {
    if (c.contents != EMPTY)
    {
      mvwaddch(stdscr, c.pos.y, c.pos.x, c.contents);
    }
  }

  wrefresh(stdscr);
  center_cursor();
}

int Board::move_cursor(const Pt<int>& pt) const
{
  return move(pt.y, pt.x);
}

int Board::move_cursor(const int& index) const
{
  if (index < 0 || index > BOARD_DIM * BOARD_DIM - 1)
  {
    // Keep to the ncurses conventions...
    return ERR;
  }
  Pt<int> dst;

  const int x = index % BOARD_DIM;
  const int y = index / BOARD_DIM;

  dst.x = START_X + (x * CELL_WIDTH) + (CELL_WIDTH / 2);
  dst.y = START_Y + (y * CELL_HEIGHT) + (CELL_HEIGHT / 2);

  return move_cursor(dst);
}

int Board::center_cursor() const
{
  return move_cursor(BOARD_DIM + 1);
}

bool Board::handle_cursor(Pt<int>* pt) const
{
  // Pt<int> pt;
  int ch;

  do
  {
    // Get a keypress from ncurses
    ch = getch();
    getyx(stdscr, pt->y, pt->x);
    switch (ch)
    {
      case 'q':
        return false;

      case KEY_LEFT:
      case 'h':
        pt->x -= CELL_WIDTH;
        if (pt->x >= min.x) move_cursor(*pt);
        break;

      case KEY_RIGHT:
      case 'l':
        pt->x += CELL_WIDTH;
        if (pt->x < max.x) move_cursor(*pt);
        break;

      case KEY_UP:
      case 'k':
        pt->y -= CELL_HEIGHT;
        if (pt->y >= min.y) move_cursor(*pt);
        break;

      case KEY_DOWN:
      case 'j':
        pt->y += CELL_HEIGHT;
        if (pt->y < max.y) move_cursor(*pt);
        break;
    }
  } while (ch != ' ');

  return true;
}

bool Board::check_win(const char& player, const int& last_move) const
{
  int col, row, diag, rdiag;
  col = row = diag = rdiag = 0;

  const int y = last_move / BOARD_DIM;
  const int x = last_move % BOARD_DIM;

  int index;
  for (int i = 0; i < BOARD_DIM; i++)
  {
    index = (y * BOARD_DIM) + i;
    if (cells[index].contents == player)
    {
      ++col;
    }

    index = (i * BOARD_DIM) + x;
    if (cells[index].contents == player)
    {
      ++row;
    }

    index = (i * BOARD_DIM) + i;
    if (cells[index].contents == player)
    {
      ++diag;
    }

    index = (i * BOARD_DIM) + (BOARD_DIM - i - 1);
    if (cells[index].contents == player)
    {
      ++rdiag;
    }
  }

  return (col == BOARD_DIM || row == BOARD_DIM || diag == BOARD_DIM ||
          rdiag == BOARD_DIM);
}

void Board::game_loop()
{
  char current_turn = X;
  int i, x, y;
  Cell* current_cell;
  Pt<int> move;
  while (handle_cursor(&move))
  {
    x = (move.x - START_X - (CELL_WIDTH / 2)) / CELL_WIDTH;
    y = (move.y - START_Y - (CELL_HEIGHT / 2)) / CELL_HEIGHT;
    i = (x * BOARD_DIM) + y;
    current_cell = &cells[i];

    switch (current_turn)
    {
      case X:
        if (current_cell->contents != EMPTY)
        {
          continue;
        }

        current_cell->contents = X;
        break;

      case O:
        if (current_cell->contents != EMPTY)
        {
          continue;
        }

        current_cell->contents = O;
        break;
    }

    if (check_win(current_turn, i))
    {
      break;
    }
    current_turn = current_turn == X ? O : X;

    draw();
  }
}