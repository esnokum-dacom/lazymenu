const char defaultsh[] = "/bin/zsh";

static const Bind explorer_items[] = {
    { XK_d, "Dolphin", "dolphin",   NULL},
    { XK_y, "Yazi",    "st yazi ~", NULL},
};

static const Menu explorer_menu = {
    explorer_items, sizeof(explorer_items) / sizeof(explorer_items[0])
};

static const Bind player_items[] = {
    { XK_p, "Prev",	"playerctl previous",   NULL},
    { XK_y, "Pause",    "playerctl play-pause", NULL},
    { XK_n, "Next",	"playerctl next", NULL},
};

static const Menu player_menu = {
    player_items, sizeof(player_items) / sizeof(player_items[0])
};
 
static const Bind root_items[] = {
    { XK_e, "Explorer",		 NULL,		 &explorer_menu  },
    { XK_p, "Player",		 NULL,		 &player_menu  },
    { XK_t, "Terminal (Root)",   "st",				 NULL },
    { XK_f, "Fetch",   "st -g 50x15 -f 'FiraMono Nerd Font:size=22' -e bash -c 'vfetch; read -n 1 -s'", NULL },
};

static const Menu root_menu = {
    root_items, sizeof(root_items) / sizeof(root_items[0])
};
