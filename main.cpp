#include <SDL2/SDL.h>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

constexpr auto memory_size = 4096;
constexpr auto register_size = 16;
constexpr auto graphics_width = 64;
constexpr auto graphics_height = 32;
constexpr auto graphics_size = graphics_width * graphics_height;
constexpr auto stack_size = 16;
constexpr auto keyboard_size = 16;
constexpr auto program_start = 0x200;
constexpr auto font_characters = 16;
constexpr auto font_height = 5;
constexpr auto font_size = font_characters * font_height;

constexpr std::uint8_t font_data[font_size] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, 0x20, 0x60, 0x20, 0x20, 0x70, 0xF0, 0x10,
    0xF0, 0x80, 0xF0, 0xF0, 0x10, 0xF0, 0x10, 0xF0, 0x90, 0x90, 0xF0, 0x10,
    0x10, 0xF0, 0x80, 0xF0, 0x10, 0xF0, 0xF0, 0x80, 0xF0, 0x90, 0xF0, 0xF0,
    0x10, 0x20, 0x40, 0x40, 0xF0, 0x90, 0xF0, 0x90, 0xF0, 0xF0, 0x90, 0xF0,
    0x10, 0xF0, 0xF0, 0x90, 0xF0, 0x90, 0x90, 0xE0, 0x90, 0xE0, 0x90, 0xE0,
    0xF0, 0x80, 0x80, 0x80, 0xF0, 0xE0, 0x90, 0x90, 0x90, 0xE0, 0xF0, 0x80,
    0xF0, 0x80, 0xF0, 0xF0, 0x80, 0xF0, 0x80, 0x80};

struct Chip8_state {
  std::uint8_t memory[memory_size] = {0};
  std::uint8_t v[register_size] = {0};
  std::uint16_t i = 0;
  std::uint16_t pc = program_start;
  std::uint16_t stack[stack_size] = {0};
  std::uint16_t sp = 0;
  std::uint8_t graphics[graphics_size];
  std::uint8_t delay_timer = 0;
  std::uint8_t sound_timer = 0;
  std::uint8_t keyboard[keyboard_size] = {0};
  std::uint8_t draw = 0;
  std::uint8_t pad[1] = {0};
};

void load_font(Chip8_state &state);

void load_font(Chip8_state &state) { memcpy(state.memory, font_data, 80); }

void load_rom(Chip8_state &state, const std::string &file_name);

void load_rom(Chip8_state &state, const std::string &file_name) {
  SDL_RWops *file = SDL_RWFromFile(file_name.c_str(), "rb");
  if (file == nullptr) {
    throw std::runtime_error("Failed to open file '" + file_name + "'.");
  }
  if (SDL_RWsize(file) < 0) {
    SDL_RWclose(file);
    throw std::runtime_error("Failed to read size of file '" + file_name +
                             "'.");
  }
  std::size_t size = static_cast<std::size_t>(SDL_RWsize(file));
  std::uint8_t *buffer = new std::uint8_t[size];
  SDL_RWread(file, buffer, size, 1);
  memcpy(&state.memory[program_start], buffer, size);
  memset(buffer, 0, size);
  delete[] buffer;
  buffer = nullptr;
  SDL_RWclose(file);
}

void process_instruction(Chip8_state &state);

void process_instruction(Chip8_state &state) {
  std::uint16_t opcode =
      static_cast<std::uint16_t>(state.memory[state.pc] << 8) |
      state.memory[state.pc + 1];
  std::uint8_t op_first = (opcode >> 12) & 0x000F;
  std::uint16_t nnn = opcode & 0x0FFF;
  std::uint8_t nn = opcode & 0x00FF;
  std::uint8_t n = opcode & 0x000F;
  std::uint8_t x = (opcode >> 8) & 0x0F;
  std::uint8_t y = (opcode >> 4) & 0x0F;
  //  std::printf("pc=%0.4x opcode=%0.4x nnn=%0.3x nn=%0.2x n=%0.1x x=%0.1x "
  //              "y=%0.1x delay_timer=%0.2x\n",
  //              state.pc, opcode, nnn, nn, n, x, y, state.delay_timer);
  state.draw = 0;
  switch (op_first) {
  case 0x00:
    switch (opcode) {
    case 0x00E0:
      memset(state.graphics, 0, sizeof(state.graphics));
      state.draw = 1;
      break;
    case 0x00EE:
      state.pc = state.stack[state.sp];
      --state.sp;
      break;
    }
    state.pc += 2;
    break;
  case 0x01:
    state.pc = nnn;
    break;
  case 0x02:
    ++state.sp;
    state.stack[state.sp] = state.pc;
    state.pc = nnn;
    break;
  case 0x03:
    state.pc += state.v[x] == nn ? 4 : 2;
    break;
  case 0x04:
    state.pc += state.v[x] != nn ? 4 : 2;
    break;
  case 0x05:
    if (state.v[x] == state.v[y]) {
      state.pc += 2;
    }
    state.pc += 2;
    break;
  case 0x06:
    state.v[x] = nn;
    state.pc += 2;
    break;
  case 0x07:
    state.v[x] += nn;
    state.pc += 2;
    break;
  case 0x08:
    switch (n) {
    case 0x00:
      state.v[x] = state.v[y];
      break;
    case 0x01:
      state.v[x] |= state.v[y];
      break;
    case 0x02:
      state.v[x] &= state.v[y];
      break;
    case 0x03:
      state.v[x] ^= state.v[y];
      break;
    case 0x04:
      state.v[0xF] = (state.v[x] + state.v[y]) > 255 ? 1 : 0;
      state.v[x] += state.v[y];
      break;
    case 0x05:
      state.v[0xF] = state.v[x] > state.v[y] ? 1 : 0;
      state.v[x] -= state.v[y];
      break;
    case 0x06:
      state.v[0xF] = state.v[x] & 0x1;
      state.v[x] >>= 1;
      break;
    case 0x07:
      state.v[0xF] = state.v[y] > state.v[x] ? 1 : 0;
      state.v[x] = state.v[y] - state.v[x];
      break;
    case 0x0E:
      state.v[0xF] = state.v[x] >> 7;
      state.v[x] <<= 1;
      break;
    }
    state.pc += 2;
    break;
  case 0x09:
    if (state.v[x] != state.v[y]) {
      state.pc += 2;
    }
    state.pc += 2;
    break;
  case 0x0A:
    state.i = nnn;
    state.pc += 2;
    break;
  case 0x0B:
    state.pc = nnn + state.v[0];
    break;
  case 0x0C: // TODO(cwink) - Replace with C++11 random number generator.
    state.v[x] = (rand() % 0xFF) & nn;
    state.pc += 2;
    break;
  case 0x0D: // TODO(cwink) - Clean this shit up.
  {
    std::uint16_t xx = state.v[(opcode & 0x0F00) >> 8];
    std::uint16_t yy = state.v[(opcode & 0x00F0) >> 4];
    std::uint16_t height = opcode & 0x000F;
    std::uint16_t pixel;
    state.v[0xF] = 0;
    for (std::uint16_t yline = 0; yline < height; yline++) {
      pixel = state.memory[state.i + yline];
      for (std::uint16_t xline = 0; xline < 8; xline++) {
        if ((pixel & (128 >> xline)) != 0) {
          if (state.graphics[(xx + xline + ((yy + yline) * graphics_width))] ==
              1) {
            state.v[0xF] = 1;
          }
          state.graphics[xx + xline + ((yy + yline) * graphics_width)] ^= 1;
        }
      }
    }
    state.draw = 1;
    state.pc += 2;
  } break;
  case 0x0E:
    switch (nn) {
    case 0x9E:
      if (state.keyboard[state.v[x]] == 1) {
        state.pc += 2;
      }
      break;
    case 0xA1:
      if (state.keyboard[state.v[x]] == 0) {
        state.pc += 2;
      }
      break;
    }
    state.pc += 2;
    break;
  case 0x0F:
    switch (nn) {
    case 0x07:
      state.v[x] = state.delay_timer;
      break;
    case 0x0A: {
      bool key_pressed = false;
      for (std::uint8_t i = 0; i < keyboard_size; ++i) {
        if (state.keyboard[i] != 0) {
          state.v[x] = i;
          key_pressed = true;
        }
      }
      if (!key_pressed) {
        return;
      }
    } break;
    case 0x15:
      state.delay_timer = state.v[x];
      break;
    case 0x18:
      state.sound_timer = state.v[x];
      break;
    case 0x1E:
      state.i += state.v[x];
      break;
    case 0x29:
      state.i = state.v[x] * font_height;
      break;
    case 0x33:
      state.memory[state.i] = state.v[x] / 100;
      state.memory[state.i + 1] = (state.v[x] / 10) % 10;
      state.memory[state.i + 2] = (state.v[x] % 100) % 10;
      break;
    case 0x55:
      for (std::uint8_t i = 0; i < x; ++i) {
        state.memory[state.i + i] = state.v[i];
      }
      state.i += x + 1;
      break;
    case 0x65:
      for (std::uint8_t i = 0; i < x; ++i) {
        state.v[i] = state.memory[state.i + i];
      }
      state.i += x + 1;
      break;
    }
    state.pc += 2;
    break;
  }
  if (state.delay_timer > 0) {
    --state.delay_timer;
  }
  if (state.sound_timer > 0) {
    --state.sound_timer;
  }
}

int main(int, char *[]);

int main(int, char *[]) {
  auto state = Chip8_state{};
  load_font(state);
  load_rom(state, "roms/TETRIS");
  SDL_Init(SDL_INIT_EVERYTHING);
  auto window = SDL_CreateWindow("Chip8 Emulator", SDL_WINDOWPOS_CENTERED,
                                 SDL_WINDOWPOS_CENTERED, graphics_width * 8,
                                 graphics_height * 8, 0);
  auto renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  SDL_RenderSetLogicalSize(renderer, graphics_width, graphics_height);
  auto running = true;
  while (running) {
    auto event = SDL_Event{};
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_WINDOWEVENT:
        switch (event.window.event) {
        case SDL_WINDOWEVENT_CLOSE:
          running = false;
          break;
        }
        break;
      }
    }
    if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_ESCAPE] == SDL_TRUE) {
      running = false;
    }
    state.keyboard[0x0] = SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_0];
    state.keyboard[0x1] = SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_1];
    state.keyboard[0x2] = SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_2];
    state.keyboard[0x3] = SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_3];
    state.keyboard[0x4] = SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_4];
    state.keyboard[0x5] = SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_5];
    state.keyboard[0x6] = SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_6];
    state.keyboard[0x7] = SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_7];
    state.keyboard[0x8] = SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_8];
    state.keyboard[0x9] = SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_9];
    state.keyboard[0xA] = SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_A];
    state.keyboard[0xB] = SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_B];
    state.keyboard[0xC] = SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_C];
    state.keyboard[0xD] = SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_D];
    state.keyboard[0xE] = SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_E];
    state.keyboard[0xF] = SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_F];
    if (state.pc >= sizeof(state.memory)) {
      break;
    }
    process_instruction(state);
    if (state.draw == 1) {
      for (std::uint8_t yy = 0; yy < 32; ++yy) {
        for (std::uint8_t xx = 0; xx < 64; ++xx) {
          if (state.graphics[yy * 64 + xx] == 0x0) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
            SDL_RenderDrawPoint(renderer, xx, yy);
          } else {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawPoint(renderer, xx, yy);
          }
        }
      }
    }
    SDL_RenderPresent(renderer);
    SDL_Delay(1);
  }
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
