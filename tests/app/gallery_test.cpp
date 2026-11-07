// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include <QDir>
#include <QFile>
#include <QImage>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include <gtest/gtest.h>

namespace tg::app {
namespace {

TEST(GalleryTest, WritesTwelveImagesAtUnitScaleWithStableSizes) {
    const QTemporaryDir folder;
    ASSERT_TRUE(folder.isValid());
    QProcess process;
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
    // The tool must override inherited desktop scaling.
    environment.insert(QStringLiteral("QT_SCALE_FACTOR"), QStringLiteral("2"));
    process.setProcessEnvironment(environment);
    process.start(QStringLiteral(TG_GALLERY), {folder.path()});
    ASSERT_TRUE(process.waitForFinished(60000));
    ASSERT_EQ(process.exitStatus(), QProcess::NormalExit);
    ASSERT_EQ(process.exitCode(), 0) << process.readAllStandardError().toStdString();
    const QDir output(folder.path());
    EXPECT_EQ(output.entryList({QStringLiteral("*.png")}, QDir::Files).size(), 12);
    for (const QString& stem : {QStringLiteral("finestra"), QStringLiteral("finestra-classica"),
                               QStringLiteral("ribbon"), QStringLiteral("proprieta"),
                               QStringLiteral("layer"), QStringLiteral("stato")}) {
        QSize lightSize;
        for (const QString& theme : {QStringLiteral("chiaro"), QStringLiteral("scuro")}) {
            const QString path =
                output.filePath(stem + QLatin1Char('-') + theme + QStringLiteral(".png"));
            SCOPED_TRACE(path.toStdString());
            const QImage image(path);
            ASSERT_FALSE(image.isNull());
            EXPECT_GT(QFile(path).size(), 0);
            if (stem.startsWith(QStringLiteral("finestra"))) {
                EXPECT_EQ(image.size(), QSize(1600, 1000));
            } else {
                EXPECT_GT(image.height(), 0);
                EXPECT_LT(image.height(), 1000);
                if (stem == QStringLiteral("ribbon") || stem == QStringLiteral("stato")) {
                    EXPECT_EQ(image.width(), 1600);
                } else {
                    EXPECT_GT(image.width(), 0);
                    EXPECT_LT(image.width(), 1600);
                }
            }
            if (theme == QStringLiteral("chiaro")) {
                lightSize = image.size();
            } else {
                EXPECT_EQ(image.size(), lightSize);
            }
        }
    }
}

TEST(GalleryTest, RejectsMissingArgumentAndUnwritableDestination) {
    const QTemporaryDir folder;
    ASSERT_TRUE(folder.isValid());
    const QString blocked = QDir(folder.path()).filePath(QStringLiteral("file"));
    QFile file(blocked);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    file.close();
    for (const QStringList& arguments : {QStringList{}, QStringList{blocked}}) {
        QProcess process;
        process.start(QStringLiteral(TG_GALLERY), arguments);
        ASSERT_TRUE(process.waitForFinished(60000));
        EXPECT_EQ(process.exitStatus(), QProcess::NormalExit);
        EXPECT_NE(process.exitCode(), 0);
    }
}

}  // namespace
}  // namespace tg::app
