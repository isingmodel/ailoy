# RogueWrite

Text-based 'rogue-lite' game inspired by '[Text Battle](https://plan9.kr/battle/)'.

You create your own character to defeat all the bosses.

Just try to play!

## Run

### with `uv`

```bash
$ uv init
$ uv run play.py
```

### without `uv`

```bash
$ pip install .
$ python play.py
```

## Language settings

`ROGUEWRITE_LANGUAGE`: Environment variable to set the language. Options: [`en`, `ko`], default is `en`.

### English

The default language is English without any setting.

or just set the environment variable explicitly:

```
ROGUEWRITE_LANGUAGE=en
```

### Korean

```
ROGUEWRITE_LANGUAGE=ko
```
