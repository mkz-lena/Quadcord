#ifndef CUSTOM_KEYBOARD_H
#define CUSTOM_KEYBOARD_H

#include "discord/types.h"
#include "ui/emoji_manager.h"
#include <citro2d.h>
#include <functional>
#include <string>
#include <vector>

namespace UI {

class CustomKeyboard {
  public:
	CustomKeyboard();
	~CustomKeyboard();

	// Returns true when finished (confirmed/cancelled). resultText & confirmed set.
	bool update(u32 kDown, u32 kHeld, u32 kUp);
	void render(C3D_RenderTarget *target);

	void open(const std::string &initial, int maxLength, bool multiline, bool password = false);
	bool isFinished() const { return finished; }
	bool isConfirmed() const { return confirmed; }
	const std::string &getResult() const { return text; }
	void reset();

	void setGuildId(const std::string &g) { guildId = g; }
	void setNitro(bool n) { hasNitro = n; }

  private:
	struct Key {
		std::string label;
		std::string insert;
		float x, y, w, h;
		bool special = false;
	};
	struct EmojiSlot {
		bool header = false;
		std::string text;
	};

	void buildLayout();
	std::vector<EmojiSlot> buildEmojiSlots();
	void handleTouch(touchPosition touch);
	void insertText(const std::string &s);
	void backspace();
	void toggleShift();
	void toggleSymbols();
	void openEmojiPanel();
	void closeEmojiPanel();

	std::string text;
	int maxLength;
	bool multiline;
	bool finished = false;
	bool confirmed = false;

	bool shift = false;
	bool symbols = false;
	bool passwordMode = false;
	bool emojiPanel = false;
	int emojiScroll = 0;
	bool emojiDragging = false;
	float lastEmojiTouchY = 0.0f;
	bool touchHandled = false;
	std::vector<EmojiSlot> cachedEmojiSlots;
	bool emojiSlotsBuilt = false;
	int emojiCursor = 0;

	std::string guildId;
	bool hasNitro = false;

	std::vector<Key> keys;
	float keyH = 30.0f;
};

} // namespace UI

#endif