#include <QApplication>
#include <QFileDialog>
#include <QDebug>
#include "lightweightvideoplayer.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // Create the video player
    LightweightVideoPlayer player;
    player.show();
    
    QString fileName;
    
    // Check if a file was passed as a command-line argument
    if (argc > 1) {
        // File path was provided (e.g., from double-clicking a video file)
        fileName = QString::fromLocal8Bit(argv[1]);
        qDebug() << "Opening file from command line:" << fileName;
    } else {
        // No file provided, show file dialog
        fileName = QFileDialog::getOpenFileName(&player,
            QObject::tr("Open Video File"),
            QString(),
            QObject::tr("Video Files (*.mp4 *.avi *.mkv *.mov *.wmv *.flv *.webm);;All Files (*.*)"));
    }
    
    if (!fileName.isEmpty()) {
        player.loadVideo(fileName);
        player.play();
    }
    
    return a.exec();
}
