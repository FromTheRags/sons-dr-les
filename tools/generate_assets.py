import os
import math
import wave
import struct
import random
from PIL import Image, ImageDraw, ImageFont

ASSETS_DIR = "assets"
IMAGES_DIR = os.path.join(ASSETS_DIR, "images")
SOUNDS_DIR = os.path.join(ASSETS_DIR, "sounds")
FONTS_DIR = os.path.join(ASSETS_DIR, "fonts")

os.makedirs(IMAGES_DIR, exist_ok=True)
os.makedirs(SOUNDS_DIR, exist_ok=True)

# Helper font loader
def get_font(size, bold=False):
    f_path = os.path.join(FONTS_DIR, "DejaVuSans-Bold.ttf" if bold else "DejaVuSans.ttf")
    if not os.path.exists(f_path):
        f_path = os.path.join(FONTS_DIR, "Graduate-Regular.ttf")
    if os.path.exists(f_path):
        try:
            return ImageFont.truetype(f_path, size)
        except Exception:
            pass
    return ImageFont.load_default()

def save_image(img, filename):
    p1 = os.path.join(IMAGES_DIR, filename)
    p2 = os.path.join(ASSETS_DIR, filename)
    img.save(p1, "BMP")
    img.save(p2, "BMP")
    print(f"Saved image: {filename}")

# ==========================================
# 1. IMAGE GENERATION (BMP)
# ==========================================

def draw_ecg(draw, y_center, width, glow_col, core_col):
    # Generate ECG pulse waveform across width
    points = []
    period = 210
    for x in range(0, width + 1):
        rel_x = x % period
        # Baseline
        dy = 0.0
        if 20 <= rel_x < 40:
            # P wave (gentle bump)
            prog = (rel_x - 20) / 20.0
            dy = -10.0 * math.sin(prog * math.pi)
        elif 50 <= rel_x < 56:
            # Q wave (small downward dip)
            prog = (rel_x - 50) / 6.0
            dy = 12.0 * math.sin(prog * math.pi)
        elif 56 <= rel_x < 68:
            # R peak (huge sharp pulse upwards!)
            prog = (rel_x - 56) / 12.0
            dy = -72.0 * math.sin(prog * math.pi)
        elif 68 <= rel_x < 76:
            # S wave (sharp dip downwards)
            prog = (rel_x - 68) / 8.0
            dy = 24.0 * math.sin(prog * math.pi)
        elif 95 <= rel_x < 125:
            # T wave (medium smooth wave)
            prog = (rel_x - 95) / 30.0
            dy = -18.0 * math.sin(prog * math.pi)
        else:
            dy = 0.0

        points.append((x, int(y_center + dy)))

    # Draw glow outer layers
    for width_px, alpha_col in [(9, (glow_col[0]//3, glow_col[1]//3, glow_col[2]//3)),
                                (5, glow_col),
                                (2, core_col)]:
        draw.line(points, fill=alpha_col, width=width_px)

def generate_menu_background():
    # 840x680 background
    w, h = 840, 680
    img = Image.new("RGB", (w, h), (24, 10, 30))
    draw = ImageDraw.Draw(img)

    # Joyful, energetic gradient: deep crimson / dark magenta to electric purple navy
    for y in range(h):
        factor = y / h
        r = int(60 * (1.0 - factor * 0.6) + 16 * factor)
        g = int(10 * (1.0 - factor) + 18 * factor)
        b = int(36 * (1.0 - factor) + 56 * factor)
        draw.line([(0, y), (w, y)], fill=(r, g, b))

    # Decorative background soundwave / equalizer grid
    for gx in range(0, w, 40):
        draw.line([(gx, 0), (gx, h)], fill=(42, 18, 48), width=1)
    for gy in range(0, h, 40):
        draw.line([(gy, 0), (gy, h)], fill=(42, 18, 48), width=1)

    # 1. Music soundwave pulsation rings (concentric acoustic pulse waves)
    cx, cy = w // 2, 355
    pulse_radii = [60, 120, 190, 270, 360, 460]
    for r_idx, pr in enumerate(pulse_radii):
        # Glow outer ring
        p_col = (180 + r_idx * 12, 30 + r_idx * 15, 90 + r_idx * 25)
        draw.ellipse([cx - pr - 2, cy - pr - 2, cx + pr + 2, cy + pr + 2], outline=(p_col[0]//3, p_col[1]//3, p_col[2]//3), width=4)
        draw.ellipse([cx - pr, cy - pr, cx + pr, cy + pr], outline=p_col, width=2)

    # 2. Pulsing ECG lines across the screen with glow (inspired by redsound beat)
    draw_ecg(draw, 355, w, (255, 40, 95), (255, 235, 245))
    draw_ecg(draw, 90, w, (190, 25, 110), (255, 190, 220))

    # Musical notes scattered playfully along the pulsating waves
    note_font = get_font(28, bold=True)
    music_notes = [
        (65, 310, "♪", (255, 215, 0)),
        (265, 300, "♫", (255, 100, 180)),
        (480, 310, "♪", (100, 230, 255)),
        (695, 300, "♫", (140, 255, 120)),
        (150, 68, "♫", (255, 230, 100)),
        (575, 68, "♪", (255, 180, 255)),
    ]
    for nx, ny, symbol, ncol in music_notes:
        draw.text((nx, ny), symbol, fill=ncol, font=note_font, anchor="mm")

    # Big Joyful Arcade Header: Title "SONS DRÔLES"
    title_font = get_font(56, bold=True)
    sub_font = get_font(18, bold=False)

    title_text = "SONS DRÔLES"
    sub_text = "♫ Le jeu de mémoire musicale ! ♫  (P: Pause musique)"

    # 3D shadow + gold gradient look
    draw.text((w // 2 + 4, 38 + 4), title_text, fill=(10, 5, 15), font=title_font, anchor="mm")
    draw.text((w // 2 + 2, 38 + 2), title_text, fill=(190, 40, 70), font=title_font, anchor="mm")
    draw.text((w // 2, 38), title_text, fill=(255, 225, 40), font=title_font, anchor="mm")
    draw.text((w // 2, 68), sub_text, fill=(255, 220, 240), font=sub_font, anchor="mm")

    # 4 Joyful & Game-Friendly ROUNDED Cards for Levels 1, 2, 3, 4
    cards = [
        # (box, title, subtitle, bg_color, border_color, accent_color, num)
        ((30, 105, 395, 330), "NIVEAU 1 : DÉBUTANT", "♫ Cloches & Carillons", (20, 45, 78), (0, 210, 255), (0, 235, 255), "1"),
        ((445, 105, 810, 330), "NIVEAU 2 : NORMAL", "★ Percussions & Groove", (68, 50, 16), (255, 200, 25), (255, 225, 55), "2"),
        ((30, 380, 395, 605), "NIVEAU 3 : DIFFICILE", "⚡ Sons Rétro & Arcade", (18, 58, 42), (20, 230, 130), (60, 255, 160), "3"),
        ((445, 380, 810, 605), "NIVEAU 4 : EXPERT", "🔥 Bruitages & Cartoon", (68, 25, 32), (255, 90, 65), (255, 130, 95), "4"),
    ]

    card_font = get_font(23, bold=True)
    card_sub_font = get_font(17, bold=False)
    num_font = get_font(64, bold=True)

    for box, title, desc, bg, border, accent, num in cards:
        x0, y0, x1, y1 = box
        # ROUNDED CARD BODY with smooth corners (radius = 26)
        draw.rounded_rectangle([x0, y0, x1, y1], radius=26, fill=bg, outline=border, width=4)

        # Upper rounded header pill inside card
        draw.rounded_rectangle([x0 + 4, y0 + 4, x1 - 4, y0 + 46], radius=22, fill=(bg[0] + 32, bg[1] + 32, bg[2] + 38))
        draw.text((x0 + 22, y0 + 25), title, fill=(255, 255, 255), font=card_font, anchor="lm")

        # Circular badge for level number
        cx_num, cy_num = x0 + 65, y0 + 130
        draw.ellipse([cx_num - 36, cy_num - 36, cx_num + 36, cy_num + 36], fill=(bg[0] + 50, bg[1] + 50, bg[2] + 60), outline=border, width=3)
        draw.text((cx_num, cy_num - 2), num, fill=accent, font=num_font, anchor="mm")

        # Subtitle & details
        draw.text((x0 + 125, y0 + 115), desc, fill=(255, 255, 255), font=card_sub_font, anchor="lm")
        draw.text((x0 + 125, y0 + 145), "5 essais • 2 réécoutes", fill=(210, 230, 250), font=get_font(15, bold=True), anchor="lm")
        draw.text((x0 + 125, y0 + 172), "8 sons à mémoriser", fill=(175, 190, 215), font=get_font(14), anchor="lm")

    # Bottom joyful tip bar
    draw.text((35, 642), "👉 Clique sur un niveau pour lancer la partie ! (Chronomètre : 2 minutes)", fill=(245, 225, 255), font=get_font(16, bold=True))

    save_image(img, "menu_background.bmp")

def generate_mode_extreme_button():
    # 295x139 button for Mode Extreme with smooth rounded corners
    w, h = 295, 139
    img = Image.new("RGB", (w, h), (24, 10, 30))
    draw = ImageDraw.Draw(img)

    # Radiant golden/purple gradient button with rounded corners
    draw.rounded_rectangle([4, 4, w - 5, h - 5], radius=28, fill=(95, 18, 115), outline=(255, 215, 0), width=4)
    draw.rounded_rectangle([10, 10, w - 11, h - 11], radius=22, fill=(125, 28, 145), outline=(255, 240, 130), width=2)

    font_ext = get_font(26, bold=True)
    font_sub = get_font(16, bold=False)

    draw.text((w // 2, 42), "MODE EXTRÊME", fill=(255, 235, 80), font=font_ext, anchor="mm")
    draw.text((w // 2, 75), "★ Débloqué ! ★", fill=(255, 255, 255), font=font_sub, anchor="mm")
    draw.text((w // 2, 105), "Ordre Aléatoire Total", fill=(225, 190, 255), font=get_font(14), anchor="mm")

    save_image(img, "mode_extreme.bmp")

def generate_aide_icon():
    # 107x106 Help/Rules button at (733, 574) with rounded pill look
    w, h = 107, 106
    img = Image.new("RGB", (w, h), (24, 10, 30))
    draw = ImageDraw.Draw(img)

    draw.rounded_rectangle([6, 6, w - 7, h - 7], radius=26, fill=(35, 85, 140), outline=(100, 200, 255), width=3)
    draw.text((w // 2, 42), "?", fill=(255, 255, 255), font=get_font(38, bold=True), anchor="mm")
    draw.text((w // 2, 78), "RÈGLES", fill=(220, 240, 255), font=get_font(15, bold=True), anchor="mm")

    save_image(img, "aide_icon.bmp")

def generate_rules_screen():
    w, h = 840, 680
    img = Image.new("RGB", (w, h), (24, 28, 42))
    draw = ImageDraw.Draw(img)

    # Outer decorative frame
    draw.rectangle([15, 15, w - 16, h - 16], fill=(30, 35, 52), outline=(70, 130, 180), width=3)
    draw.rectangle([25, 25, w - 26, 85], fill=(40, 50, 75), outline=(100, 160, 220), width=2)

    title_font = get_font(34, bold=True)
    draw.text((w // 2, 55), "RÈGLES DU JEU : SONS DRÔLES", fill=(255, 220, 80), font=title_font, anchor="mm")

    h_font = get_font(20, bold=True)
    t_font = get_font(17, bold=False)

    lines = [
        (110, "1. BUT DU JEU", (100, 200, 255)),
        (135, "Retrouver l'ordre exact de la séquence de 8 sons jouée au début de la manche.", (240, 240, 240)),
        (175, "2. DÉROULEMENT", (100, 200, 255)),
        (200, "• Dès le début du niveau, la musique se coupe pour écouter la séquence de 8 sons.", (240, 240, 240)),
        (228, "• Chaque case de la grille correspond à un son : cliquez pour reconstituer la mélodie.", (240, 240, 240)),
        (256, "• Chaque clic numérote la case choisie (1, 2, 3 ... jusqu'à 8).", (240, 240, 240)),
        (284, "• Recliquez sur la dernière case jouée pour l'annuler en cas d'erreur.", (240, 240, 240)),
        (324, "3. ESSAIS DE COMBINAISONS & RÉÉCOUTES", (100, 200, 255)),
        (349, "• Vous disposez de 5 essais de combinaisons complètes de 8 cases !", (255, 220, 100)),
        (377, "• Si une série de 8 est inexacte, la grille se vide et vous perdez 1 essai.", (240, 240, 240)),
        (405, "• Vous disposez de 2 réécoutes de la séquence cible (bouton Replay en haut à droite).", (255, 200, 120)),
        (433, "• Le chronomètre décompte 2 minutes pour réussir la grille.", (255, 200, 120)),
        (473, "4. MODE EXTRÊME", (100, 200, 255)),
        (498, "• Triomphez des 4 premiers niveaux pour débloquer le Mode Extrême !", (255, 235, 100)),
        (526, "• En mode extrême, l'ordre et la disposition des cases sont 100% aléatoires.", (240, 240, 240)),
        (566, "5. RACCOURCIS CLAVIER", (100, 200, 255)),
        (591, "• F11 ou Alt+Entrée : Plein écran   |   Touche P : Pause audio   |   Échap : Retour menu", (180, 215, 255)),
    ]

    for y_pos, text, col in lines:
        if text.startswith(("1.", "2.", "3.", "4.", "5.")):
            draw.text((50, y_pos), text, fill=col, font=h_font)
        else:
            draw.text((65, y_pos), text, fill=col, font=t_font)

    # Bottom note
    draw.text((w // 2, 642), "Cliquez sur la flèche Retour en haut à gauche pour revenir au menu principal", fill=(160, 180, 200), font=get_font(15), anchor="mm")

    save_image(img, "rules_background.bmp")

def generate_game_tiles():
    # 8 cases: 209 x 339 each
    tw, th = 209, 339

    # Unrevealed Tile with Question Mark
    img_unrev = Image.new("RGB", (tw, th), (30, 36, 54))
    draw = ImageDraw.Draw(img_unrev)
    # Beveled 3D look
    draw.rectangle([0, 0, tw - 1, th - 1], fill=(36, 44, 66), outline=(60, 75, 110), width=2)
    draw.line([(0, 0), (tw - 1, 0)], fill=(80, 100, 145), width=2)
    draw.line([(0, 0), (0, th - 1)], fill=(80, 100, 145), width=2)
    draw.line([(tw - 1, 0), (tw - 1, th - 1)], fill=(20, 25, 40), width=2)
    draw.line([(0, th - 1), (tw - 1, th - 1)], fill=(20, 25, 40), width=2)

    # Center circle for Question Mark
    draw.ellipse([tw // 2 - 45, th // 2 - 55, tw // 2 + 45, th // 2 + 35], fill=(48, 58, 88), outline=(85, 110, 160), width=2)
    draw.text((tw // 2, th // 2 - 10), "?", fill=(210, 225, 250), font=get_font(52, bold=True), anchor="mm")

    save_image(img_unrev, "case_interrogation.bmp")

    # Revealed Tiles 1..8 with distinctive colors & speaker wave symbol
    tile_colors = [
        ("case_sound_1.bmp", (0, 171, 255), "Bleu"),
        ("case_sound_2.bmp", (127, 216, 0), "Vert pomme"),
        ("case_sound_3.bmp", (255, 221, 0), "Jaune"),
        ("case_sound_4.bmp", (30, 80, 255), "Bleu marine"),
        ("case_sound_5.bmp", (162, 0, 255), "Violet"),
        ("case_sound_6.bmp", (255, 60, 180), "Rose"),
        ("case_sound_7.bmp", (255, 135, 0), "Orange"),
        ("case_sound_8.bmp", (220, 30, 30), "Rouge"),
    ]

    for fname, base_col, label in tile_colors:
        timg = Image.new("RGB", (tw, th), (25, 28, 40))
        tdraw = ImageDraw.Draw(timg)

        # Gradient background tinted with base color
        for y in range(th):
            factor = y / th
            r = int(25 + base_col[0] * 0.25 * factor)
            g = int(28 + base_col[1] * 0.25 * factor)
            b = int(40 + base_col[2] * 0.25 * factor)
            tdraw.line([(0, y), (tw, y)], fill=(r, g, b))

        # Border with theme color
        tdraw.rectangle([0, 0, tw - 1, th - 1], outline=base_col, width=3)

        # Speaker symbol in center
        cx = tw // 2
        cy = th // 2 - 20
        # Speaker body
        tdraw.rectangle([cx - 24, cy - 14, cx - 12, cy + 14], fill=base_col)
        tdraw.polygon([(cx - 12, cy - 14), (cx + 8, cy - 28), (cx + 8, cy + 28), (cx - 12, cy + 14)], fill=base_col)
        # Sound waves arcs
        tdraw.arc([cx + 12, cy - 18, cx + 24, cy + 18], start=-60, end=60, fill=(255, 255, 255), width=3)
        tdraw.arc([cx + 18, cy - 28, cx + 36, cy + 28], start=-60, end=60, fill=(255, 255, 255), width=3)

        # Bottom label bar
        tdraw.rectangle([6, th - 44, tw - 7, th - 8], fill=(20, 24, 34), outline=base_col, width=1)
        tdraw.text((tw // 2, th - 26), "CASE ACTIVE", fill=(240, 245, 255), font=get_font(14, bold=True), anchor="mm")

        save_image(timg, fname)

def generate_buttons():
    # 1. Retour button (60x60)
    b_ret = Image.new("RGB", (60, 60), (35, 42, 60))
    d_ret = ImageDraw.Draw(b_ret)
    d_ret.rectangle([0, 0, 59, 59], outline=(80, 140, 220), width=2)
    # Left arrow
    d_ret.polygon([(14, 30), (34, 16), (34, 24), (48, 24), (48, 36), (34, 36), (34, 44)], fill=(120, 200, 255))
    save_image(b_ret, "btn_retour.bmp")

    # 2. Replay button (68x60)
    b_rep = Image.new("RGB", (68, 60), (35, 42, 60))
    d_rep = ImageDraw.Draw(b_rep)
    d_rep.rectangle([0, 0, 67, 59], outline=(100, 210, 140), width=2)
    # Circular reload arrow
    d_rep.arc([16, 12, 52, 48], start=30, end=300, fill=(120, 230, 160), width=4)
    d_rep.polygon([(46, 10), (56, 18), (44, 26)], fill=(120, 230, 160))
    save_image(b_rep, "btn_replay.bmp")

    # 3. Croix disabled / chances over (62x60)
    b_crx = Image.new("RGB", (62, 60), (45, 20, 25))
    d_crx = ImageDraw.Draw(b_crx)
    d_crx.rectangle([0, 0, 61, 59], outline=(220, 60, 70), width=2)
    d_crx.line([(15, 15), (46, 45)], fill=(255, 70, 80), width=5)
    d_crx.line([(15, 45), (46, 15)], fill=(255, 70, 80), width=5)
    save_image(b_crx, "croix.bmp")

    # 4. App icon (sdl_icone.bmp 32x32) and sons.ico
    icon_img = Image.new("RGBA", (48, 48), (0, 0, 0, 0))
    d_icon = ImageDraw.Draw(icon_img)
    # Disc
    d_icon.ellipse([2, 2, 45, 45], fill=(30, 40, 70), outline=(255, 215, 0), width=2)
    # Musical note
    d_icon.ellipse([10, 28, 20, 38], fill=(255, 215, 0))
    d_icon.ellipse([24, 24, 34, 34], fill=(255, 215, 0))
    d_icon.line([(19, 32), (19, 12)], fill=(255, 215, 0), width=3)
    d_icon.line([(33, 28), (33, 8)], fill=(255, 215, 0), width=3)
    d_icon.line([(18, 12), (34, 8)], fill=(255, 215, 0), width=4)

    # Save BMP 32x32 for SDL_WM_SetIcon
    bmp_icon = icon_img.resize((32, 32), Image.Resampling.LANCZOS).convert("RGB")
    save_image(bmp_icon, "sdl_icone.bmp")

    # Save ICO
    ico_path = os.path.join(ASSETS_DIR, "sons.ico")
    icon_img.save(ico_path, format="ICO", sizes=[(16, 16), (32, 32), (48, 48)])
    icon_img.save("sons.ico", format="ICO", sizes=[(16, 16), (32, 32), (48, 48)])
    print("Saved sons.ico")


# ==========================================
# 2. AUDIO SYNTHESIS GENERATION (WAV)
# ==========================================

SAMPLE_RATE = 44100

def write_wav(filename, samples):
    p1 = os.path.join(SOUNDS_DIR, filename)
    p2 = os.path.join(ASSETS_DIR, filename)
    for p in [p1, p2]:
        with wave.open(p, 'wb') as wf:
            wf.setnchannels(1)  # Mono
            wf.setsampwidth(2)  # 16-bit
            wf.setframerate(SAMPLE_RATE)
            raw = bytearray()
            for s in samples:
                val = max(-32767, min(32767, int(s * 32767.0)))
                raw.extend(struct.pack('<h', val))
            wf.writeframes(raw)
    print(f"Generated WAV: {filename} ({len(samples)/SAMPLE_RATE:.2f}s)")

# Audio building blocks
def adsr(t, total_len, a=0.01, d=0.05, s=0.7, r=0.1):
    if t < a:
        return t / a
    elif t < a + d:
        return 1.0 - (1.0 - s) * ((t - a) / d)
    elif t < total_len - r:
        return s
    else:
        return s * (1.0 - (t - (total_len - r)) / r)

def tone_harmonic(freq, duration, harmonics=[(1.0, 1.0), (2.0, 0.4), (3.0, 0.15)], a=0.01, d=0.1, s=0.6, r=0.15):
    n = int(SAMPLE_RATE * duration)
    samples = []
    for i in range(n):
        t = i / SAMPLE_RATE
        env = adsr(t, duration, a, d, s, r)
        val = 0.0
        for h_mult, h_amp in harmonics:
            val += h_amp * math.sin(2.0 * math.pi * freq * h_mult * t)
        val *= 0.6 * env
        samples.append(val)
    return samples

def fm_synth(carrier, mod_freq, mod_idx, duration, a=0.01, d=0.08, s=0.5, r=0.15):
    n = int(SAMPLE_RATE * duration)
    samples = []
    for i in range(n):
        t = i / SAMPLE_RATE
        env = adsr(t, duration, a, d, s, r)
        mod = mod_idx * math.sin(2.0 * math.pi * mod_freq * t)
        val = 0.6 * env * math.sin(2.0 * math.pi * carrier * t + mod)
        samples.append(val)
    return samples

def pitch_sweep(start_f, end_f, duration, wave_type="sine", a=0.01, d=0.05, s=0.7, r=0.1):
    n = int(SAMPLE_RATE * duration)
    samples = []
    phase = 0.0
    for i in range(n):
        t = i / SAMPLE_RATE
        cur_f = start_f + (end_f - start_f) * (t / duration)
        phase += 2.0 * math.pi * cur_f / SAMPLE_RATE
        env = adsr(t, duration, a, d, s, r)

        if wave_type == "sine":
            val = math.sin(phase)
        elif wave_type == "saw":
            val = 2.0 * (phase / (2.0 * math.pi) - math.floor(phase / (2.0 * math.pi) + 0.5))
        elif wave_type == "square":
            val = 1.0 if math.sin(phase) >= 0 else -1.0
        else:
            val = math.sin(phase)

        samples.append(val * 0.6 * env)
    return samples

def noise_burst(duration, a=0.005, d=0.05, s=0.2, r=0.1, tone_freq=0):
    n = int(SAMPLE_RATE * duration)
    samples = []
    random.seed(42)
    for i in range(n):
        t = i / SAMPLE_RATE
        env = adsr(t, duration, a, d, s, r)
        noise = (random.random() * 2.0 - 1.0)
        if tone_freq > 0:
            val = 0.4 * noise + 0.6 * math.sin(2.0 * math.pi * tone_freq * t)
        else:
            val = noise
        samples.append(val * 0.5 * env)
    return samples

def generate_all_sounds():
    # 1. UI SFX
    # Buzzer (Error on wrong click)
    s_buzz = []
    dur = 0.35
    for i in range(int(SAMPLE_RATE * dur)):
        t = i / SAMPLE_RATE
        env = adsr(t, dur, a=0.01, d=0.05, s=0.8, r=0.1)
        v = 0.5 * (math.sin(2 * math.pi * 130 * t) + 0.5 * math.sin(2 * math.pi * 138 * t))
        v += 0.3 * (1.0 if math.sin(2 * math.pi * 65 * t) >= 0 else -1.0)
        s_buzz.append(v * 0.7 * env)
    write_wav("buzzer.wav", s_buzz)

    # Undo / cancellation sound
    s_undo = pitch_sweep(240, 680, 0.22, "sine", a=0.01, d=0.04, s=0.6, r=0.08)
    write_wav("undo.wav", s_undo)

    # Background ambient music loop (Soft 8-measure chord progression loop ~12 sec)
    tempo = 110.0
    beat_dur = 60.0 / tempo
    total_beats = 16
    total_dur = total_beats * beat_dur
    total_samples = int(SAMPLE_RATE * total_dur)
    music_samples = [0.0] * total_samples

    # Chord progression: Cmaj7 -> Am7 -> Fmaj7 -> G7
    # 4 beats each
    chords = [
        ([261.6, 329.6, 392.0, 493.8], 0.0),   # C E G B
        ([220.0, 261.6, 329.6, 392.0], 4.0),   # A C E G
        ([174.6, 220.0, 261.6, 329.6], 8.0),   # F A C E
        ([196.0, 246.9, 293.6, 349.2], 12.0),  # G B D F
    ]

    for notes, start_beat in chords:
        for b_offset in range(4):
            beat_time = (start_beat + b_offset) * beat_dur
            # Arpeggiated electric piano notes
            note_freq = notes[b_offset % len(notes)]
            dur_note = beat_dur * 0.95
            tone = tone_harmonic(note_freq, dur_note, [(1.0, 0.5), (2.0, 0.2), (3.0, 0.08)], a=0.02, d=0.15, s=0.4, r=0.2)
            start_idx = int(beat_time * SAMPLE_RATE)
            for k in range(len(tone)):
                if start_idx + k < total_samples:
                    music_samples[start_idx + k] += tone[k] * 0.25

            # Bass note on beat 0 and 2
            if b_offset in (0, 2):
                bass_freq = notes[0] / 2.0
                bass_tone = tone_harmonic(bass_freq, beat_dur * 1.5, [(1.0, 0.6), (2.0, 0.3)], a=0.03, d=0.2, s=0.5, r=0.3)
                for k in range(len(bass_tone)):
                    if start_idx + k < total_samples:
                        music_samples[start_idx + k] += bass_tone[k] * 0.35

    # Normalize music
    max_m = max(abs(s) for s in music_samples) or 1.0
    music_samples = [s / max_m * 0.6 for s in music_samples]
    write_wav("bg_music.wav", music_samples)

    # 32 Unique Gameplay Sounds for 4 Levels
    # LEVEL 1: Melodic Bells & Tonal Instruments
    l1_notes = [523.25, 587.33, 659.25, 698.46, 783.99, 880.00, 987.77, 1046.50]  # C5 to C6
    for idx, f in enumerate(l1_notes, start=1):
        # Crystal Bell with high harmonics
        bell = tone_harmonic(f, 0.65, [(1.0, 0.6), (2.76, 0.3), (5.4, 0.15), (8.2, 0.08)], a=0.005, d=0.15, s=0.3, r=0.4)
        write_wav(f"sound_1_{idx}.wav", bell)

    # LEVEL 2: Percussions & Rhythms
    # 1: Water drop
    s_drop = pitch_sweep(1400, 700, 0.25, "sine", a=0.005, d=0.05, s=0.5, r=0.1)
    write_wav("sound_2_1.wav", s_drop)
    # 2: Kick drum (Sub impact)
    s_kick = pitch_sweep(160, 45, 0.35, "sine", a=0.002, d=0.08, s=0.4, r=0.2)
    write_wav("sound_2_2.wav", s_kick)
    # 3: Snare crack
    s_snare = noise_burst(0.28, a=0.002, d=0.06, s=0.25, r=0.15, tone_freq=180)
    write_wav("sound_2_3.wav", s_snare)
    # 4: Metallic cymbal / hi-hat
    s_hihat = noise_burst(0.35, a=0.001, d=0.08, s=0.15, r=0.2, tone_freq=0)
    write_wav("sound_2_4.wav", s_hihat)
    # 5: Boing comic spring
    s_boing = fm_synth(220, 18, 12, 0.45, a=0.01, d=0.1, s=0.6, r=0.2)
    write_wav("sound_2_5.wav", s_boing)
    # 6: Clap
    s_clap = noise_burst(0.22, a=0.003, d=0.04, s=0.2, r=0.1, tone_freq=450)
    write_wav("sound_2_6.wav", s_clap)
    # 7: Woodblock
    s_wood = tone_harmonic(850, 0.18, [(1.0, 0.7), (2.4, 0.3)], a=0.002, d=0.03, s=0.2, r=0.1)
    write_wav("sound_2_7.wav", s_wood)
    # 8: Dual whistle
    s_whistle = []
    w_dur = 0.4
    for i in range(int(SAMPLE_RATE * w_dur)):
        t = i / SAMPLE_RATE
        env = adsr(t, w_dur, a=0.02, d=0.05, s=0.8, r=0.1)
        mod = 1.0 + 0.1 * math.sin(2 * math.pi * 20 * t)
        v = (math.sin(2 * math.pi * 2200 * mod * t) + math.sin(2 * math.pi * 2350 * mod * t)) * 0.4 * env
        s_whistle.append(v)
    write_wav("sound_2_8.wav", s_whistle)

    # LEVEL 3: Sci-Fi & Electronic Arcade SFX
    # 1: Laser blast
    s_laser = pitch_sweep(2400, 180, 0.3, "saw", a=0.005, d=0.05, s=0.5, r=0.15)
    write_wav("sound_3_1.wav", s_laser)
    # 2: 8-bit arcade arpeggio
    s_8bit = []
    n_step = int(SAMPLE_RATE * 0.08)
    for freq in [330, 440, 550, 660]:
        for i in range(n_step):
            t = i / SAMPLE_RATE
            v = (1.0 if math.sin(2 * math.pi * freq * t) >= 0 else -1.0) * 0.4
            s_8bit.append(v)
    write_wav("sound_3_2.wav", s_8bit)
    # 3: Space teleport
    s_teleport = fm_synth(440, 35, 18, 0.55, a=0.05, d=0.15, s=0.6, r=0.2)
    write_wav("sound_3_3.wav", s_teleport)
    # 4: Electric zap
    s_zap = noise_burst(0.28, a=0.005, d=0.06, s=0.5, r=0.12, tone_freq=320)
    write_wav("sound_3_4.wav", s_zap)
    # 5: Robot ring mod
    s_robot = []
    dur_rob = 0.45
    for i in range(int(SAMPLE_RATE * dur_rob)):
        t = i / SAMPLE_RATE
        env = adsr(t, dur_rob, a=0.02, d=0.08, s=0.7, r=0.15)
        carrier = math.sin(2 * math.pi * 380 * t)
        modulator = math.sin(2 * math.pi * 75 * t)
        s_robot.append(carrier * modulator * 0.6 * env)
    write_wav("sound_3_5.wav", s_robot)
    # 6: Power-up fanfare
    s_powerup = pitch_sweep(300, 1200, 0.38, "sine", a=0.01, d=0.08, s=0.7, r=0.15)
    write_wav("sound_3_6.wav", s_powerup)
    # 7: Submarine sonar ping
    s_sonar = tone_harmonic(1480, 0.6, [(1.0, 0.8), (2.0, 0.1)], a=0.005, d=0.2, s=0.2, r=0.3)
    write_wav("sound_3_7.wav", s_sonar)
    # 8: Pulsed forcefield
    s_shield = fm_synth(320, 80, 8, 0.45, a=0.02, d=0.1, s=0.6, r=0.15)
    write_wav("sound_3_8.wav", s_shield)

    # LEVEL 4: Nature & Comic Animals
    # 1: Bird chirping
    s_bird = []
    dur_b = 0.45
    for i in range(int(SAMPLE_RATE * dur_b)):
        t = i / SAMPLE_RATE
        env = adsr(t, dur_b, a=0.02, d=0.1, s=0.6, r=0.15)
        f = 2400 + 900 * math.sin(2 * math.pi * 28 * t)
        s_bird.append(math.sin(2 * math.pi * f * t) * 0.5 * env)
    write_wav("sound_4_1.wav", s_bird)
    # 2: Frog ribbit
    s_frog = []
    dur_f = 0.35
    for i in range(int(SAMPLE_RATE * dur_f)):
        t = i / SAMPLE_RATE
        env = adsr(t, dur_f, a=0.01, d=0.08, s=0.6, r=0.1)
        saw = 2.0 * ((t * 120) % 1.0) - 1.0
        mod = 1.0 if math.sin(2 * math.pi * 35 * t) >= 0 else 0.2
        s_frog.append(saw * mod * 0.5 * env)
    write_wav("sound_4_2.wav", s_frog)
    # 3: Bee buzz
    s_bee = tone_harmonic(220, 0.45, [(1.0, 0.5), (2.0, 0.3), (3.0, 0.2)], a=0.05, d=0.05, s=0.8, r=0.1)
    write_wav("sound_4_3.wav", s_bee)
    # 4: Wind gust
    s_wind = noise_burst(0.6, a=0.15, d=0.15, s=0.5, r=0.25, tone_freq=90)
    write_wav("sound_4_4.wav", s_wind)
    # 5: Fire crackle
    s_fire = noise_burst(0.4, a=0.01, d=0.1, s=0.4, r=0.15, tone_freq=600)
    write_wav("sound_4_5.wav", s_fire)
    # 6: Thunder rumble
    s_thunder = pitch_sweep(120, 40, 0.7, "sine", a=0.05, d=0.2, s=0.5, r=0.3)
    write_wav("sound_4_6.wav", s_thunder)
    # 7: Heartbeat double thud
    s_heart = []
    dur_h = 0.5
    for i in range(int(SAMPLE_RATE * dur_h)):
        t = i / SAMPLE_RATE
        env1 = adsr(t, 0.2, a=0.01, d=0.08, s=0.3, r=0.08) if t < 0.2 else 0.0
        env2 = adsr(t - 0.22, 0.2, a=0.01, d=0.08, s=0.3, r=0.08) if t >= 0.22 and t < 0.42 else 0.0
        v = math.sin(2 * math.pi * 60 * t) * (env1 * 0.7 + env2 * 0.6)
        s_heart.append(v)
    write_wav("sound_4_7.wav", s_heart)
    # 8: Cartoon laughter staccato
    s_laugh = []
    dur_l = 0.5
    for i in range(int(SAMPLE_RATE * dur_l)):
        t = i / SAMPLE_RATE
        env = adsr(t, dur_l, a=0.02, d=0.1, s=0.7, r=0.15)
        burst = 1.0 if (int(t * 12) % 2 == 0) else 0.1
        f = 500 + 150 * math.sin(2 * math.pi * 6 * t)
        s_laugh.append(math.sin(2 * math.pi * f * t) * burst * 0.5 * env)
    write_wav("sound_4_8.wav", s_laugh)

if __name__ == "__main__":
    print("Generating free-to-use graphics (BMP)...")
    generate_menu_background()
    generate_mode_extreme_button()
    generate_aide_icon()
    generate_rules_screen()
    generate_game_tiles()
    generate_buttons()

    print("\nGenerating free-to-use audio (WAV)...")
    generate_all_sounds()

    print("\nAll assets generated successfully!")
