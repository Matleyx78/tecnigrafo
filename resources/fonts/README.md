# Liberation Sans Regular

Bundled unmodified from the official Liberation Fonts **2.1.5** binary archive:

- Release: https://github.com/liberationfonts/liberation-fonts/releases/tag/2.1.5
- Archive: https://github.com/liberationfonts/liberation-fonts/files/7261482/liberation-fonts-ttf-2.1.5.tar.gz
- License: SIL Open Font License 1.1, copied verbatim from the archive's `LICENSE` to [OFL.txt](OFL.txt).
- Upstream license: https://github.com/liberationfonts/liberation-fonts/blob/2.1.5/LICENSE

SHA-256:

```text
7191c669bf38899f73a2094ed00f7b800553364f90e2637010a69c0e268f25d0  liberation-fonts-ttf-2.1.5.tar.gz
76d04c18ea243f426b7de1f3ad208e927008f961dc5945e5aad352d0dfde8ee8  LiberationSans-Regular.ttf
```

`tools/fontmetrics` reads this TTF during the build and generates the table used
by `core`. Capital-letter height is one unit; advances are added without kerning.
See [ADR 0013](../../docs/adr/0013-testo-e-font.md).
