#include "ui/custom_keyboard.h"
#include "core/config.h"
#include "log.h"
#include "core/i18n.h"
#include "discord/discord_client.h"
#include "ui/emoji_manager.h"
#include "ui/image_manager.h"
#include "ui/screen_manager.h"
#include "utils/utf8_utils.h"
#include <3ds.h>
#include <algorithm>
#include <citro2d.h>
#include <cmath>
#include <string>

namespace UI {

CustomKeyboard::CustomKeyboard() { buildLayout(); }
CustomKeyboard::~CustomKeyboard() {}

void CustomKeyboard::reset() {
	text.clear();
	finished = false;
	confirmed = false;
	shift = false;
	symbols = false;
	passwordMode = false;
	emojiPanel = false;
	emojiScroll = 0;
}

void CustomKeyboard::open(const std::string &initial, int maxLength, bool multiline, bool password) {
	reset();
	text = initial;
	this->maxLength = (maxLength < 0 || maxLength == 0) ? 2000 : maxLength;
	this->multiline = multiline;
	this->passwordMode = password;
	buildLayout();
}

void CustomKeyboard::buildLayout() {
	keys.clear();
	float rowH = 26.0f;
	float gap = 2.0f;
	float top = 80.0f;

	auto addKey = [&](const std::string &label, const std::string &insert, float x, float yy, float w, float h, bool special) {
		Key k; k.label = label; k.insert = insert; k.x = x; k.y = yy; k.w = w; k.h = h; k.special = special;
		keys.push_back(k);
	};

	if (symbols) {
		// number/symbol rows
		const char *r1 = "1234567890";
		const char *r2 = "@#$%&*()-+";
		const char *r3 = ".,!?;:'\"/\\";
		float x = 2.0f;
		for (const char *p = r1; *p; ++p) { std::string s(1,*p); addKey(s,s,x,top,30.0f,rowH,false); x+=30.0f+gap; }
		x = 2.0f + 15.0f;
		for (const char *p = r2; *p; ++p) { std::string s(1,*p); addKey(s,s,x,top+rowH+gap,30.0f,rowH,false); x+=30.0f+gap; }
		x = 2.0f;
		addKey("Abc", "", x, top+2*(rowH+gap), 40.0f, rowH, true);
		x += 40.0f+gap;
		for (const char *p = r3; *p; ++p) { std::string s(1,*p); addKey(s,s,x,top+2*(rowH+gap),30.0f,rowH,false); x+=30.0f+gap; }
		addKey("Bk", "", x, top+2*(rowH+gap), 40.0f, rowH, true);
	} else {
		const char *row1 = "qwertyuiop";
		const char *row2 = "asdfghjkl";
		const char *row3 = "zxcvbnm";
		float x = 2.0f;
		for (const char *p = row1; *p; ++p) { std::string s(1,*p); addKey(s,s,x,top,30.0f,rowH,false); x+=30.0f+gap; }
		x = 2.0f + 15.0f;
		for (const char *p = row2; *p; ++p) { std::string s(1,*p); addKey(s,s,x,top+rowH+gap,30.0f,rowH,false); x+=30.0f+gap; }
		x = 2.0f;
		addKey("Sh", "", x, top+2*(rowH+gap), 40.0f, rowH, true);
		x += 40.0f+gap;
		for (const char *p = row3; *p; ++p) { std::string s(1,*p); addKey(s,s,x,top+2*(rowH+gap),30.0f,rowH,false); x+=30.0f+gap; }
		addKey("Bk", "", x, top+2*(rowH+gap), 40.0f, rowH, true);
	}

	float by = top + 3*(rowH+gap);
	addKey(symbols ? "Abc" : "123", "", 2.0f, by, 46.0f, rowH, true);
	addKey(" ", " ", 2.0f+46.0f+gap, by, 150.0f, rowH, false);
	addKey("Em", "", 2.0f+46.0f+150.0f+2*gap, by, 36.0f, rowH, true);
	addKey("Ent", "\n", 2.0f+46.0f+150.0f+36.0f+3*gap, by, 46.0f, rowH, true);

	float cy = by + rowH + gap;
	addKey("Cancel", "", 2.0f, cy, 90.0f, rowH, true);
	addKey("OK", "", 2.0f+90.0f+gap, cy, 220.0f, rowH, true);
}

bool CustomKeyboard::update(u32 kDown, u32 kHeld, u32 kUp) {
	if (finished) return true;
	if (emojiPanel) {
		touchPosition touch; hidTouchRead(&touch);
		if (kDown & KEY_TOUCH && !touchHandled) {
			if (touch.py >= 40) {
				emojiDragging = true;
				lastEmojiTouchY = touch.py;
			} else {
				emojiDragging = false;
			}
			handleTouch(touch);
			touchHandled = true;
		} else if (kHeld & KEY_TOUCH) {
			if (emojiDragging) {
				float dy = touch.py - lastEmojiTouchY;
				emojiScroll -= (int)dy;
				lastEmojiTouchY = touch.py;
			}
		} else if (kUp & KEY_TOUCH) {
			emojiDragging = false;
			touchHandled = false;
		}
		// clamp
		// compute total height
		auto slots = buildEmojiSlots();
		float cell = 28.0f; int cols = 11;
		float y = 0; int col = 0;
		for (auto &sl : slots) {
			if (sl.header) { if (col != 0){y+=cell;col=0;} y+=cell; }
			else { col++; if (col>=cols){col=0;y+=cell;} }
		}
		if (col != 0) y += cell;
		float totalH = y;
		float viewH = 200.0f;
		if (emojiScroll < 0) emojiScroll = 0;
		if (totalH > viewH && emojiScroll > (int)(totalH - viewH)) emojiScroll = (int)(totalH - viewH);
	} else {
		if (kDown & KEY_TOUCH && !touchHandled) {
			touchPosition touch; hidTouchRead(&touch);
			handleTouch(touch);
			touchHandled = true;
		}
		if (kUp & KEY_TOUCH) {
			touchHandled = false;
		}
	}
	if (kDown & KEY_B) { finished = true; confirmed = false; }
	if (kDown & KEY_START) { finished = true; confirmed = true; }
	return finished;
}

void CustomKeyboard::handleTouch(touchPosition touch) {
	if (emojiPanel) {
		if (touch.py < 40) { closeEmojiPanel(); return; }
		auto slots = buildEmojiSlots();
		// find slot under touch
		float cell = 28.0f; int cols = 11;
		float gridW = cols*cell; float xOffset = (320.0f - gridW)/2.0f;
		float y = 40.0f - (float)emojiScroll;
		int col = 0;
		for (auto &sl : slots) {
			if (sl.header) {
				if (col != 0) { y += cell; col = 0; }
				// header row
				if (touch.py >= y && touch.py < y + cell && touch.px >= 0 && touch.px <= 320) {
					return; // header not selectable
				}
				y += cell;
				continue;
			}
			float sx = xOffset + col*cell;
			float sy = y;
			if (touch.px >= sx && touch.px < sx+cell && touch.py >= sy && touch.py < sy+cell) {
				// insert if allowed
				bool isCustom = (!sl.text.empty() && sl.text[0] == '<');
				if (isCustom && !hasNitro) {
					return; // can't use
				}
				insertText(sl.text);
				Config::getInstance().addRecentEmoji(sl.text);
				closeEmojiPanel();
				return;
			}
			col++;
			if (col >= cols) { col = 0; y += cell; }
		}
		return;
	}

	for (auto &k : keys) {
		if (touch.px >= k.x && touch.px <= k.x+k.w && touch.py >= k.y && touch.py <= k.y+k.h) {
			if (k.label == "Sh") toggleShift();
			else if (k.label == "123" || k.label == "Abc") toggleSymbols();
			else if (k.label == "Em") openEmojiPanel();
			else if (k.label == "Cancel") { finished = true; confirmed = false; }
			else if (k.label == "OK") { finished = true; confirmed = true; }
			else if (k.label == "Bk") backspace();
			else {
				std::string ins = k.insert;
				if (ins.empty()) ins = k.label;
				if (!symbols && !k.special && ins.size()==1 && ins[0]>='a' && ins[0]<='z' && shift) {
					ins[0] = ins[0]-'a'+'A';
				}
				if (ins == "\n" && !multiline) { finished = true; confirmed = true; return; }
				insertText(ins);
			}
			return;
		}
	}
}

void CustomKeyboard::insertText(const std::string &s) {
	if (maxLength > 0 && (int)(text.size() + s.size()) > maxLength) {
		// trim? allow until max
		int room = maxLength - (int)text.size();
		if (room <= 0) return;
		text += s.substr(0, room);
		return;
	}
	text += s;
}

void CustomKeyboard::backspace() {
	if (text.empty()) return;
	size_t cursor = 0, lastStart = 0;
	while (cursor < text.size()) { lastStart = cursor; Utils::Utf8::decodeNext(text, cursor); }
	text.erase(lastStart);
}

void CustomKeyboard::toggleShift() { shift = !shift; }
void CustomKeyboard::toggleSymbols() { symbols = !symbols; buildLayout(); }
void CustomKeyboard::openEmojiPanel() { emojiPanel = true; emojiScroll = 0; emojiSlotsBuilt = false; touchHandled = false; }
void CustomKeyboard::closeEmojiPanel() { emojiPanel = false; emojiSlotsBuilt = false; }

std::vector<CustomKeyboard::EmojiSlot> CustomKeyboard::buildEmojiSlots() {
	if (emojiSlotsBuilt) {
		return cachedEmojiSlots;
	}
	std::vector<EmojiSlot> slots;
	// Recent
	auto recents = Config::getInstance().getRecentEmojis();
	Logger::log("[KB] recents=%zu", recents.size());
	if (!recents.empty()) {
		EmojiSlot h; h.header = true; h.text = "Recent"; slots.push_back(h);
		for (auto &e : recents) { EmojiSlot s; s.text = e; slots.push_back(s); }
	}
	// All guilds' custom emojis, current first
	Discord::DiscordClient &client = Discord::DiscordClient::getInstance();
	std::vector<std::string> guildOrder;
	if (!guildId.empty() && guildId != "DM") guildOrder.push_back(guildId);
	for (auto &g : client.getGuilds()) {
		if (g.id == guildId) continue;
		guildOrder.push_back(g.id);
	}
	for (auto &gid : guildOrder) {
		Discord::Guild g = client.getGuild(gid);
		Logger::log("[KB] guild=%s emojis=%zu", g.name.c_str(), g.emojis.size());
		if (g.emojis.empty()) continue;
		EmojiSlot h; h.header = true; h.text = g.name.empty() ? "Server" : g.name; slots.push_back(h);
		for (auto &em : g.emojis) {
			EmojiSlot s; s.text = "<" + std::string(em.animated ? "a" : "") + ":" + em.name + ":" + em.id + ">";
			slots.push_back(s);
		}
	}
	// Stock (twemoji)
	{
		EmojiSlot h; h.header = true; h.text = "Stock"; slots.push_back(h);
		static std::vector<std::string> stock;
		if (stock.empty()) {
			UI::EmojiManager::getInstance().init();
			auto &cps = UI::EmojiManager::getInstance().getCodepoints();
			Logger::log("[KB] codepoints=%zu", cps.size());
			for (auto &hex : cps) {
				stock.push_back(Utils::Utf8::hexToUtf8(hex));
			}
			if (stock.empty()) {
				// Fallback common emojis
				const char *common[] = {"😀","😁","😂","🤣","😃","😄","😅","😆","😉","😊","😋","😎","😍","😘","🥰","😗","😙","😚","🙂","🤗","🤩","🤔","🤨","😐","😑","😶","🙄","😏","😣","😥","😮","🤐","😯","😪","😫","🥱","😴","😌","😛","😜","😝","🤤","😒","😓","😔","😕","🙃","🤑","😲","☹️","🙁","😖","😞","😟","😤","😢","😭","😦","😧","😨","😩","🤯","😬","😰","😱","🥵","🥶","😳","🤪","😵","😡","😠","🤬","😷","🤒","🤕","🤢","🤮","🤧","😇","🤠","🤡","🥳","🥸","😎","🤓","🧐"};
				for (auto c : common) stock.push_back(c);
			}
		}
		for (auto &e : stock) { EmojiSlot s; s.text = e; slots.push_back(s); }
	}
	cachedEmojiSlots = slots;
	emojiSlotsBuilt = true;
	return cachedEmojiSlots;
}

void CustomKeyboard::render(C3D_RenderTarget *target) {
	C2D_DrawRectSolid(0, 0, 0.0f, 320, 240, ScreenManager::colorBackgroundDark());

	// preview
	C2D_DrawRectSolid(4, 4, 0.0f, 312, 70, ScreenManager::colorBackgroundLight());
	std::string preview = text;
	if (passwordMode) { preview.assign(preview.size(), '*'); }
	if (preview.size() > 200) preview = preview.substr(preview.size()-200);
	drawText(8, 8, 0.1f, 0.4f, 0.4f, ScreenManager::colorText(), preview);

	if (emojiPanel) {
		C2D_DrawRectSolid(0, 0, 0.0f, 320, 40, ScreenManager::colorBackgroundLight());
		drawText(8, 12, 0.1f, 0.45f, 0.45f, ScreenManager::colorText(), "Emojis");
		auto slots = buildEmojiSlots();
		float cell = 28.0f; int cols = 11;
		float gridW = cols*cell; float xOffset = (320.0f - gridW)/2.0f;
		float y = 40.0f - (float)emojiScroll;
		int col = 0;
		bool any = false;
		for (auto &sl : slots) {
			if (sl.header) {
				if (col != 0) { y += cell; col = 0; }
				if (y + cell > 0 && y < 240) {
					drawText(8, y + 8, 0.1f, 0.4f, 0.4f, ScreenManager::colorTextMuted(), sl.text);
				}
				y += cell;
				continue;
			}
			any = true;
			float sx = xOffset + col*cell;
			float sy = y;
			if (sy + cell > 0 && sy < 240) {
				std::string e = sl.text;
				if (!e.empty() && e[0] == '<') {
					size_t second = e.find(':', e.find(':')+1);
					size_t close = e.find('>', second);
					if (second != std::string::npos && close != std::string::npos) {
						std::string id = e.substr(second+1, close-second-1);
						bool anim = (e.size()>1 && e[1]=='a');
						UI::EmojiManager::getInstance().prefetchEmoji(id, anim);
						UI::EmojiManager::EmojiInfo info = UI::EmojiManager::getInstance().getEmojiInfo(id, anim);
						if (info.tex) {
							float scale = (cell-4)/info.originalW;
							Tex3DS_SubTexture sub = {(u16)info.originalW,(u16)info.originalH,0.0f,1.0f,
								(float)info.originalW/info.tex->width, 1.0f-(float)info.originalH/info.tex->height};
							C2D_DrawImageAt({info.tex,&sub}, sx+2, sy+2, 0.1f, nullptr, scale, scale);
							UI::ImageManager::getInstance().markDrawn(info.animation);
						}
					}
				} else {
					std::string hex = Utils::Utf8::utf8ToHex(e);
					UI::EmojiManager::EmojiInfo info = UI::EmojiManager::getInstance().getTwemojiInfo(hex);
					if (info.tex) {
						float scale = (cell-4)/info.originalW;
						Tex3DS_SubTexture sub = {(u16)info.originalW,(u16)info.originalH,0.0f,1.0f,
							(float)info.originalW/info.tex->width, 1.0f-(float)info.originalH/info.tex->height};
						C2D_DrawImageAt({info.tex,&sub}, sx+2, sy+2, 0.1f, nullptr, scale, scale);
					} else {
						drawText(sx+4, sy+8, 0.1f, 0.35f, 0.35f, ScreenManager::colorText(), e);
					}
				}
			}
			col++;
			if (col >= cols) { col = 0; y += cell; }
		}
		if (!any) {
			auto &cps = UI::EmojiManager::getInstance().getCodepoints();
			char buf[64];
			snprintf(buf, sizeof(buf), "No emojis yet (cp=%zu)", cps.size());
			drawText(8, 60, 0.1f, 0.4f, 0.4f, ScreenManager::colorTextMuted(), buf);
		}
		return;
	}

	for (auto &k : keys) {
		u32 bg = k.special ? ScreenManager::colorBackgroundLight() : ScreenManager::colorInput();
		C2D_DrawRectSolid(k.x, k.y, 0.05f, k.w, k.h, bg);
		std::string lab = k.label;
		if (!symbols && !k.special && lab.size()==1 && lab[0]>='a' && lab[0]<='z' && shift) {
			lab[0] = lab[0]-'a'+'A';
		}
		drawText(k.x+4, k.y+8, 0.1f, 0.4f, 0.4f, ScreenManager::colorText(), lab);
	}
}

} // namespace UI