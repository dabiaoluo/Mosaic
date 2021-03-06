#include <mosaic/Mosaic.h>

namespace mos {

  Mosaic::Mosaic()
   :mosaic_tex(0)
  {
  }

  Mosaic::~Mosaic() {
    shutdown();
  }

  int Mosaic::init() {

    int r;

    /* get the texture sizes for the mosaic. */
    int mw = fex::config.getMosaicImageWidth();
    int mh = fex::config.getMosaicImageHeight();
    if (0 > mw) {
      return -100;
    }
    if (0 > mh) {
      return -101;
    }

    /* initialize the video input. */
    r = video_input.init();
    if (0 != r) {
      return r;
    }
    
    /* initialize the feature extractor and comparator. */
    r = featurex.init(video_input.texid());
    if (0 != r) {
      return r;
    }

    /* create the mosaic texture */
    glGenTextures(1, &mosaic_tex);
    glBindTexture(GL_TEXTURE_2D, mosaic_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mw, mh, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    /* initialize the async uploader */
    if (0 != async_upload.init(mw, mh, GL_RGBA)) {
      return -102;
    }

    return 0;
  }

  void Mosaic::update() {

    /* update the input texture - is used by the gpu analyzer.*/
    video_input.update();

    /* new input, update gpu analyzer. */
    if (video_input.needsUpdate()) {
      featurex.analyzeGPU();
      featurex.match();
      
      if (NULL != featurex.mosaic_pixels) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mosaic_tex);
        async_upload.upload(featurex.mosaic_pixels);
      }
    }
  }

  void Mosaic::draw() {
    draw(0, 0, mos::config.window_width, mos::config.window_height);
  }

  void Mosaic::draw(int x, int y, int w, int h) {

#if !defined(NDEBUG)
    if (0 == w || 0 == h) {
      RX_ERROR("Invalid width or height: %d x %d", w, h);
      exit(EXIT_FAILURE);
    }
#endif

    /* draw the result. */
    painter.clear();
    painter.texture(mosaic_tex, x, y + h, w, -h);
    painter.draw();
  }

  void Mosaic::debugDraw() {
    video_input.draw();
  }

  int Mosaic::shutdown() {

    int r = 0;
    int result = 0;

    if (0 != mosaic_tex) {
      glDeleteTextures(1, &mosaic_tex);
    }

    r = async_upload.shutdown();
    if (r != 0) {
      RX_WARNING("Error while trying to shutdown the async uploader.");
      result = -1;
    }

    r = video_input.shutdown();
    if (0 != r) {
      RX_WARNING("Error while trying to shutdown the video input");
      result = -2;
    }

    r = featurex.shutdown();
    if (0 != r) {
      RX_WARNING("Error while trying to shutdown the feature extractor");
      result = -3;
    }

    return result;
  }

} /* namespace mos */
