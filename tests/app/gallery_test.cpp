// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include <gtest/gtest.h>

#include <string>

namespace tg::app {
namespace {

// QProcess needs an application object, and gtest_main creates none.
class GalleryTest : public ::testing::Test {
private:
    int m_argc = 1;
    std::string m_name = "tg_app_gallery_tests";
    char* m_argv = m_name.data();
    QCoreApplication m_application{m_argc, &m_argv};
};

TEST_F(GalleryTest, WritesFourteenImagesAtUnitScaleWithStableSizes) {
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
    EXPECT_EQ(output.entryList({QStringLiteral("*.png")}, QDir::Files).size(), 14);
    QStringList stems;
    stems.append(QStringLiteral("finestra"));
    stems.append(QStringLiteral("finestra-classica"));
    stems.append(QStringLiteral("ribbon"));
    stems.append(QStringLiteral("proprieta"));
    stems.append(QStringLiteral("layer"));
    stems.append(QStringLiteral("stato"));
    // The ribbon of a window with the frame of the system (docs/tappe/
    // 0.7.5/P40, point 5).
    stems.append(QStringLiteral("ribbon-cornice-sistema"));
    for (const QString& stem : stems) {
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
                    // The window without frame keeps 2 px on the left and on
                    // the right, where it is resized (SARibbonMainWindow).
                    EXPECT_EQ(image.width(), 1596);
                } else if (stem == QStringLiteral("ribbon-cornice-sistema")) {
                    // The system draws the frame, outside the window.
                    EXPECT_GE(image.width(), 1596);
                    EXPECT_LE(image.width(), 1600);
                } else {
                    EXPECT_GT(image.width(), 0);
                    EXPECT_LT(image.width(), 1600);
                }
            }
            // Themes change colours only: equal sizes also show that the
            // second window did not inherit the first one's saved layout.
            if (theme == QStringLiteral("chiaro")) {
                lightSize = image.size();
            } else {
                EXPECT_EQ(image.size(), lightSize);
            }
        }
    }
}

TEST_F(GalleryTest, RejectsMissingArgumentAndUnwritableDestination) {
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
