/*********************************************************************************
 *                                                                               *
 * Copyright (c) 2017, Dr. Daniel L. Lau                                         *
 * All rights reserved.                                                          *
 *                                                                               *
 * Redistribution and use in source and binary forms, with or without            *
 * modification, are permitted provided that the following conditions are met:   *
 * 1. Redistributions of source code must retain the above copyright             *
 *    notice, this list of conditions and the following disclaimer.              *
 * 2. Redistributions in binary form must reproduce the above copyright          *
 *    notice, this list of conditions and the following disclaimer in the        *
 *    documentation and/or other materials provided with the distribution.       *
 * 3. All advertising materials mentioning features or use of this software      *
 *    must display the following acknowledgement:                                *
 *    This product includes software developed by the <organization>.            *
 * 4. Neither the name of the <organization> nor the                             *
 *    names of its contributors may be used to endorse or promote products       *
 *    derived from this software without specific prior written permission.      *
 *                                                                               *
 * THIS SOFTWARE IS PROVIDED BY Dr. Daniel L. Lau ''AS IS'' AND ANY              *
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED     *
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE        *
 * DISCLAIMED. IN NO EVENT SHALL Dr. Daniel L. Lau BE LIABLE FOR ANY             *
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES    *
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;  *
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND   *
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT    *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS *
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                  *
 *                                                                               *
 *********************************************************************************/

#include "lauvideoglwidget.h"
#include "locale.h"

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUVideoGLWidget::~LAUVideoGLWidget()
{
    if (wasInitialized()) {
        makeCurrent();
        if (videoTexture) {
            delete videoTexture;
        }
        vertexArrayObject.release();
    }
    qDebug() << QString("LAUVideoGLWidget::~LAUVideoGLWidget()");
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUVideoGLWidget::setFrame(int cols, int rows, QOpenGLTexture::PixelFormat format, QOpenGLTexture::PixelType type, unsigned char *buffer)
{
    // REPORT FRAME RATE TO THE CONSOLE
    counter++;
    if (counter >= 30) {
        qDebug() << QString("%1 fps").arg(1000.0 * (float)counter / (float)time.elapsed());
        time.restart();
        counter = 0;
    }

    // MAKE THIS THE CURRENT OPENGL CONTEXT
    makeCurrent();

    // SEE IF WE NEED A NEW TEXTURE TO HOLD THE INCOMING VIDEO FRAME
    if (!videoTexture || videoTexture->width() != cols || videoTexture->height() != rows) {
        if (videoTexture) {
            delete videoTexture;
        }

        // CREATE THE GPU SIDE TEXTURE BUFFER TO HOLD THE INCOMING VIDEO
        videoTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
        videoTexture->setSize(cols, rows);
        videoTexture->setFormat(QOpenGLTexture::RGBA32F);
        videoTexture->setWrapMode(QOpenGLTexture::ClampToBorder);
        videoTexture->setMinificationFilter(QOpenGLTexture::Nearest);
        videoTexture->setMagnificationFilter(QOpenGLTexture::Nearest);
        videoTexture->allocateStorage();
    }

    // UPLOAD THE CPU BUFFER TO THE GPU TEXTURE
    // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
    videoTexture->setData(format, type, (const void *)buffer);

    // PROCESS THE TEXTURE
    process();

    // UPDATE THE USER DISPLAY
    update();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUVideoGLWidget::setFrame(const QVideoFrame &frame)
{
    QVideoFrame localFrame = frame;

    if (localFrame.map(QAbstractVideoBuffer::ReadOnly)) {
        // REPORT FRAME RATE TO THE CONSOLE
        counter++;
        if (counter >= 30) {
            qDebug() << QString("%1 fps").arg(1000.0 * (float)counter / (float)time.elapsed());
            time.restart();
            counter = 0;
        }

        makeCurrent();

        // SEE IF WE NEED A NEW TEXTURE TO HOLD THE INCOMING VIDEO FRAME
        if (!videoTexture || videoTexture->width() != localFrame.width() || videoTexture->height() != localFrame.height()) {
            if (videoTexture) {
                delete videoTexture;
            }

            // CREATE THE GPU SIDE TEXTURE BUFFER TO HOLD THE INCOMING VIDEO
            videoTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
            videoTexture->setSize(localFrame.width(), localFrame.height());
            videoTexture->setFormat(QOpenGLTexture::RGBA32F);
            videoTexture->setWrapMode(QOpenGLTexture::ClampToBorder);
            videoTexture->setMinificationFilter(QOpenGLTexture::Nearest);
            videoTexture->setMagnificationFilter(QOpenGLTexture::Nearest);
            videoTexture->allocateStorage();
        }

        // UPLOAD THE CPU BUFFER TO THE GPU TEXTURE
        // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
        QVideoFrame::PixelFormat format = localFrame.pixelFormat();
        if (format == QVideoFrame::Format_ARGB32) {
            unsigned int bytesPerSample = localFrame.bytesPerLine() / localFrame.width() / 4;
            if (bytesPerSample == sizeof(unsigned char)) {
                videoTexture->setData(QOpenGLTexture::BGRA, QOpenGLTexture::UInt8, (const void *)localFrame.bits());
            }
        } else if (format == QVideoFrame::Format_RGB32){
            unsigned int bytesPerSample = localFrame.bytesPerLine() / localFrame.width() / 4;
            if (bytesPerSample == sizeof(unsigned char)) {
                videoTexture->setData(QOpenGLTexture::BGRA, QOpenGLTexture::UInt8, (const void *)localFrame.bits());
            }
        }
        localFrame.unmap();

        // PROCESS THE TEXTURE
        process();

        // UPDATE THE USER DISPLAY
        update();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUVideoGLWidget::setFrame(QImage frame)
{
    if (frame.isNull() == false) {
        // REPORT FRAME RATE TO THE CONSOLE
        counter++;
        if (counter >= 30) {
            qDebug() << QString("%1 fps").arg(1000.0 * (float)counter / (float)time.elapsed());
            time.restart();
            counter = 0;
        }

        makeCurrent();

        // SEE IF WE NEED A NEW TEXTURE TO HOLD THE INCOMING VIDEO FRAME
        if (!videoTexture || videoTexture->width() != frame.width() || videoTexture->height() != frame.height()) {
            if (videoTexture) {
                delete videoTexture;
            }

            // CREATE THE GPU SIDE TEXTURE BUFFER TO HOLD THE INCOMING VIDEO
            videoTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
            videoTexture->setSize(frame.width(), frame.height());
            videoTexture->setFormat(QOpenGLTexture::RGBA32F);
            videoTexture->setWrapMode(QOpenGLTexture::ClampToBorder);
            videoTexture->setMinificationFilter(QOpenGLTexture::Nearest);
            videoTexture->setMagnificationFilter(QOpenGLTexture::Nearest);
            videoTexture->allocateStorage();

            qDebug() << videoTexture->width() << videoTexture->height();
        }

        // UPLOAD THE CPU BUFFER TO THE GPU TEXTURE
        // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
        QImage::Format format = frame.format();
        if (format == QImage::Format_ARGB32) {
            unsigned int bytesPerSample = frame.bytesPerLine() / frame.width() / 4;
            if (bytesPerSample == sizeof(unsigned char)) {
                videoTexture->setData(QOpenGLTexture::BGRA, QOpenGLTexture::UInt8, (const void *)frame.bits());
            }
        }

        // PROCESS THE TEXTURE
        process();

        // UPDATE THE USER DISPLAY
        update();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUVideoGLWidget::initialize()
{
    // INITIALIZE OUR GL CALLS AND SET THE CLEAR COLOR
    initializeOpenGLFunctions();
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    // CREATE THE VERTEX ARRAY OBJECT FOR FEEDING VERTICES TO OUR SHADER PROGRAMS
    vertexArrayObject.create();
    vertexArrayObject.bind();

    // CREATE VERTEX BUFFER TO HOLD CORNERS OF QUADRALATERAL
    quadVertexBuffer = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    quadVertexBuffer.create();
    quadVertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    if (quadVertexBuffer.bind()) {
        // ALLOCATE THE VERTEX BUFFER FOR HOLDING THE FOUR CORNERS OF A RECTANGLE
        quadVertexBuffer.allocate(16 * sizeof(float));
        float *buffer = (float *)quadVertexBuffer.map(QOpenGLBuffer::WriteOnly);
        if (buffer) {
            buffer[0]  = -1.0;
            buffer[1]  = -1.0;
            buffer[2]  = 0.0;
            buffer[3]  = 1.0;
            buffer[4]  = +1.0;
            buffer[5]  = -1.0;
            buffer[6]  = 0.0;
            buffer[7]  = 1.0;
            buffer[8]  = +1.0;
            buffer[9]  = +1.0;
            buffer[10] = 0.0;
            buffer[11] = 1.0;
            buffer[12] = -1.0;
            buffer[13] = +1.0;
            buffer[14] = 0.0;
            buffer[15] = 1.0;
            quadVertexBuffer.unmap();
        } else {
            qDebug() << QString("quadVertexBuffer not allocated.") << glGetError();
        }
        quadVertexBuffer.release();
    }

    // CREATE INDEX BUFFER TO ORDERINGS OF VERTICES FORMING POLYGON
    quadIndexBuffer = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    quadIndexBuffer.create();
    quadIndexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    if (quadIndexBuffer.bind()) {
        quadIndexBuffer.allocate(6 * sizeof(unsigned int));
        unsigned int *indices = (unsigned int *)quadIndexBuffer.map(QOpenGLBuffer::WriteOnly);
        if (indices) {
            indices[0] = 0;
            indices[1] = 1;
            indices[2] = 2;
            indices[3] = 0;
            indices[4] = 2;
            indices[5] = 3;
            quadIndexBuffer.unmap();
        } else {
            qDebug() << QString("indiceBufferA buffer mapped from GPU.");
        }
        quadIndexBuffer.release();
    }

    // CREATE SHADER FOR SHOWING THE VIDEO NOT AVAILABLE IMAGE
    setlocale(LC_NUMERIC, "C");
    program.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/shaders/Shaders/displayRGBVideo.vert");
    program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/Shaders/displayRGBVideo.frag");
    program.link();
    setlocale(LC_ALL, "");
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUVideoGLWidget::resize(int w, int h)
{
    // Get the Desktop Widget so that we can get information about multiple monitors connected to the system.
    QDesktopWidget *dkWidget = QApplication::desktop();
    QList<QScreen *> screenList = QGuiApplication::screens();
    qreal devicePixelRatio = screenList[dkWidget->screenNumber(this)]->devicePixelRatio();
    localHeight = h * devicePixelRatio;
    localWidth = w * devicePixelRatio;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUVideoGLWidget::paint()
{
    // SET THE VIEW PORT AND CLEAR THE SCREEN BUFFER
    glViewport(0, 0, localWidth, localHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // MAKE SURE WE HAVE A TEXTURE TO SHOW
    if (videoTexture) {
        if (program.bind()) {
            if (quadVertexBuffer.bind()) {
                if (quadIndexBuffer.bind()) {
                    // SET THE ACTIVE TEXTURE ON THE GPU
                    glActiveTexture(GL_TEXTURE0);
                    videoTexture->bind();
                    program.setUniformValue("qt_texture", 0);

                    // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                    program.setAttributeBuffer("qt_vertex", GL_FLOAT, 0, 4, 4 * sizeof(float));
                    program.enableAttributeArray("qt_vertex");

                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                    quadIndexBuffer.release();
                }
                quadVertexBuffer.release();
            }
            program.release();
        }
    }
}
