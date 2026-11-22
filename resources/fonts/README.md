# Liberation Sans

Bundled unmodified from the official Liberation Fonts **2.1.5** binary archive:

- Release: https://github.com/liberationfonts/liberation-fonts/releases/tag/2.1.5
- Archive: https://github.com/liberationfonts/liberation-fonts/files/7261482/liberation-fonts-ttf-2.1.5.tar.gz
- License: SIL Open Font License 1.1, copied verbatim from the archive's `LICENSE` to [OFL.txt](OFL.txt).
- Upstream license: https://github.com/liberationfonts/liberation-fonts/blob/2.1.5/LICENSE

SHA-256:

```text
7191c669bf38899f73a2094ed00f7b800553364f90e2637010a69c0e268f25d0  liberation-fonts-ttf-2.1.5.tar.gz
76d04c18ea243f426b7de1f3ad208e927008f961dc5945e5aad352d0dfde8ee8  LiberationSans-Regular.ttf
788abee4c806d660e8aee46689dd8540cd4bb98da03dcc9d171ce3efd99a9173  LiberationSans-Bold.ttf
e5bae5c4cde31f22142753855f4f8fb86da6ff39955ed3c0a11248b0d16948b0  LiberationSans-Italic.ttf
698da70fc191cc5f33ad4d6d3fe830fe4624b898ea2e3169955928b7c491f1ee  LiberationSans-BoldItalic.ttf
```

`tools/fontmetrics` reads the Regular TTF during the build and generates the
table used by `core`. Capital-letter height is one unit; advances are added
without kerning. See [ADR 0013](../../docs/adr/0013-testo-e-font.md).

The Bold, Italic and BoldItalic files come from the same archive for the
formatted text of stage 0.8.1
([ADR 0054](../../docs/adr/0054-testo-formattato-ed-editor.md), point 2); until
package 0.8.1-PT6 they are not read by the build.
