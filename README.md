# Lazy Menu
This little piece of code for X11 is an useful tool based on this [neovim plugin](https://github.com/folke/which-key.nvim)

It works fine catching the inputs and executing commands.

You can specify submenus and his functions
## Dependencies
- libx11
- Latest c compiler
- Make

# Build
Simply clone and then run the make command
```
git clone https://github.com/esnokum-dacom/lazymenu
cd lazymenu
make
```

# install
After cloned the repo you simply do
```
sudo make clean install
```

## Usage
The usage is basic, only run
```
lm
```

and select the options.

# Configure

You can configure the keybinds, menus, names, etc. In `config.def.h`

```c
static const Bind root_items[] = {
    { XK_h, "Hello World",	    "echo 'Hello world'",		 NULL  },
};
```

This is the basic functionallity.

You can run shell commands. And then show them in the terminal or in your notify-manager like dunst.

#### Sub Menus

```c
static const Bind root_items[] = {
    { XK_h, "Sub Menu",	    NULL,		 &PointerToSubmenu},
};
```

Notice about the command section is `NULL`, because if it have a command you're going to experiment something called keybind freeze.

So I recommend you to dont touch it because I don't know if It can happens


```c
static const Bind submenuitems[] = {
    { XK_h, "Hello submenu",	    "echo 'Hello Submenu'", NULL},
};
```

Here you write the commands which is going to have the submenu.

```
static const Menu PointerToSubmenu = {
    submenuitems, sizeof(submenuitems) / sizeof(submenuitems[0])
};
```

And here you can specify which items is going to have inside

# FEATURES
It can be modable and execute sh commands
