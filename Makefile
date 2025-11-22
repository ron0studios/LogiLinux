CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -ljpeg
TARGETS = hid_monitor tictactoe lcd_display tictactoe_lcd gif_player navigation_grid

all: $(TARGETS)

hid_monitor: hid_monitor.cpp
	$(CXX) $(CXXFLAGS) -o hid_monitor hid_monitor.cpp

tictactoe: tictactoe.cpp
	$(CXX) $(CXXFLAGS) -o tictactoe tictactoe.cpp

lcd_display: lcd_display.cpp
	$(CXX) $(CXXFLAGS) -o lcd_display lcd_display.cpp

tictactoe_lcd: tictactoe_lcd.cpp
	$(CXX) $(CXXFLAGS) -o tictactoe_lcd tictactoe_lcd.cpp

gif_player: gif_player.cpp
	$(CXX) $(CXXFLAGS) -o gif_player gif_player.cpp

navigation_grid: navigation_grid.cpp
	$(CXX) $(CXXFLAGS) -o navigation_grid navigation_grid.cpp -pthread

giffer: giffer.cpp
	$(CXX) $(CXXFLAGS) -o giffer giffer.cpp

clean:
	rm -f $(TARGETS)

run-monitor: hid_monitor
	sudo ./hid_monitor

run-tictactoe: tictactoe
	sudo ./tictactoe

run-lcd: lcd_display
	sudo ./lcd_display

run-tictactoe-lcd: tictactoe_lcd
	sudo ./tictactoe_lcd

run-gif: gif_player
	sudo ./gif_player

run-nav: navigation_grid
	sudo ./navigation_grid

run: run-tictactoe-lcd

.PHONY: all clean run run-monitor run-tictactoe run-lcd run-tictactoe-lcd run-gif run-nav
