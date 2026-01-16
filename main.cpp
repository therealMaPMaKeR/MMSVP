#include <QApplication>
#include <QFileDialog>
#include "lightweightvideoplayer.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // Create the video player
    LightweightVideoPlayer player;
    player.show();
    
    // Optionally, open a file dialog to select a video
    QString fileName = QFileDialog::getOpenFileName(&player,
        QObject::tr("Open Video File"),
        QString(),
        QObject::tr("Video Files (*.mp4 *.avi *.mkv *.mov *.wmv *.flv *.webm);;All Files (*.*)"));
    
    if (!fileName.isEmpty()) {
        player.loadVideo(fileName);
        player.play();
    }
    
    return a.exec();
}
