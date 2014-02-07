/****************************************************************************
 Author: Luma (stubma@gmail.com)
 
 https://github.com/stubma/cocos2dx-better
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID

#include "CCImage_richlabel.h"
#include <jni.h>
#include "JniHelper.h"

using namespace std;

NS_CC_BEGIN

class CLBitmapDC {
public:
	// link meta list
	LinkMetaList m_linkMetas;

	// shadow stroke padding
	CCPoint m_shadowStrokePadding;

public:
	CLBitmapDC() :
			m_pData(NULL), m_nWidth(0), m_nHeight(0), m_shadowStrokePadding(CCPointZero) {
	}

	~CLBitmapDC(void) {
	}

	bool getBitmapFromJavaShadowStroke(const char *text, int nWidth, int nHeight, CCImage::ETextAlign eAlignMask, const char * pFontName, float fontSize,
			float textTintR = 1.0, float textTintG = 1.0, float textTintB = 1.0, bool shadow = false, float shadowDeltaX = 0.0, float shadowDeltaY = 0.0, int shadowColor = 0,
			float shadowBlur = 0.0, bool stroke = false, float strokeColorR = 0.0, float strokeColorG = 0.0, float strokeColorB =
					0.0, float strokeSize = 0.0, bool sizeOnly = false) {
		JniMethodInfo methodInfo;
		if(!JniHelper::getStaticMethodInfo(methodInfo, "org/cocos2dx/lib/RichLabelBitmap", "createRichLabelBitmap",
				"(Ljava/lang/String;Ljava/lang/String;IFFFIIIZFFIFZFFFFFZ)V")) {
			CCLOG("%s %d: error to get methodInfo", __FILE__, __LINE__);
			return false;
		}

		// Do a full lookup for the font path using CCFileUtils in case the given font name is a relative path to a font file asset,
		// or the path has been mapped to a different location in the app package:
		std::string fullPathOrFontName = CCFileUtils::sharedFileUtils()->fullPathForFilename(pFontName);

		// If the path name returned includes the 'assets' dir then that needs to be removed, because the android.content.Context
		// requires this portion of the path to be omitted for assets inside the app package.
		if(fullPathOrFontName.find("assets/") == 0) {
			fullPathOrFontName = fullPathOrFontName.substr(strlen("assets/")); // Chop out the 'assets/' portion of the path.
		}

		/**create bitmap
		 * this method call Cococs2dx.createBitmap()(java code) to create the bitmap, the java code
		 * will call Java_org_cocos2dx_lib_Cocos2dxBitmap_nativeInitBitmapDC() to init the width, height
		 * and data.
		 * use this approach to decrease the jni call number
		 */
		jstring jstrText = methodInfo.env->NewStringUTF(text);
		jstring jstrFont = methodInfo.env->NewStringUTF(fullPathOrFontName.c_str());

		methodInfo.env->CallStaticVoidMethod(methodInfo.classID, methodInfo.methodID, jstrText, jstrFont, (int) fontSize, textTintR, textTintG, textTintB,
				eAlignMask, nWidth, nHeight, shadow, shadowDeltaX, -shadowDeltaY, shadowColor, shadowBlur, stroke, strokeColorR, strokeColorG, strokeColorB, strokeSize, CC_CONTENT_SCALE_FACTOR(), sizeOnly);

		methodInfo.env->DeleteLocalRef(jstrText);
		methodInfo.env->DeleteLocalRef(jstrFont);
		methodInfo.env->DeleteLocalRef(methodInfo.classID);

		return true;
	}

	bool getBitmapFromJava(const char *text, int nWidth, int nHeight, CCImage::ETextAlign eAlignMask, const char * pFontName, float fontSize) {
		return getBitmapFromJavaShadowStroke(text, nWidth, nHeight, eAlignMask, pFontName, fontSize);
	}

	// ARGB -> RGBA
	inline unsigned int swapAlpha(unsigned int value) {
		return ((value << 8 & 0xffffff00) | (value >> 24 & 0x000000ff));
	}

public:
	int m_nWidth;
	int m_nHeight;
	JNIEnv *env;

	// this data will pass to CCImage, so it should NOT be released in this class
	unsigned char *m_pData;
};

static CLBitmapDC& sharedCLBitmapDC() {
	static CLBitmapDC s_BmpDC;
	return s_BmpDC;
}

// swap the alpha channel in an 32 bit image (from ARGB to RGBA)
static void swapAlphaChannel(unsigned int *pImageMemory, unsigned int numPixels) {
	for(int c = 0; c < numPixels; ++c, ++pImageMemory) {
		// copy the current pixel
		unsigned int currenPixel = (*pImageMemory);

		// swap channels and store back
		char *pSource = (char *) &currenPixel;
		*pImageMemory = (pSource[0] << 24) | (pSource[3] << 16) | (pSource[2] << 8) | pSource[1];
	}
}

CCImage_richlabel::CCImage_richlabel() {
}

CCImage_richlabel::~CCImage_richlabel() {
}

bool CCImage_richlabel::initWithRichStringShadowStroke(const char * pText, int nWidth, int nHeight, ETextAlign eAlignMask, const char * pFontName, int nSize,
		float textTintR, float textTintG, float textTintB, bool shadow, float shadowOffsetX, float shadowOffsetY, int shadowColor, float shadowBlur,
		bool stroke, float strokeR, float strokeG, float strokeB, float strokeSize) {
	bool bRet = false;
	do {
		CC_BREAK_IF(!pText);

		CLBitmapDC &dc = sharedCLBitmapDC();

		CC_BREAK_IF(!dc.getBitmapFromJavaShadowStroke(pText,
                                                      nWidth,
                                                      nHeight,
                                                      eAlignMask,
                                                      pFontName,
                                                      nSize,
                                                      textTintR,
                                                      textTintG,
                                                      textTintB,
                                                      shadow,
                                                      shadowOffsetX,
                                                      shadowOffsetY,
                                                      shadowColor,
                                                      shadowBlur,
                                                      stroke,
                                                      strokeR,
                                                      strokeG,
                                                      strokeB,
                                                      strokeSize,
                                                      false));

		// assign the dc.m_pData to m_pData in order to save time
		m_pData = dc.m_pData;

		CC_BREAK_IF(!m_pData);

		m_nWidth = (short) dc.m_nWidth;
		m_nHeight = (short) dc.m_nHeight;
		m_bHasAlpha = true;
		m_bPreMulti = true;
		m_nBitsPerComponent = 8;

		// swap the alpha channel (ARGB to RGBA)
		swapAlphaChannel((unsigned int *)m_pData, (m_nWidth * m_nHeight));

		// ok
		bRet = true;

		// save info needed by label
		m_linkMetas = dc.m_linkMetas;
		m_shadowStrokePadding = dc.m_shadowStrokePadding;
	} while(0);

	return bRet;
}

CCSize CCImage_richlabel::measureRichString(const char* pText,
                                            const char* pFontName,
                                            int nSize,
                                            int maxWidth,
                                            float shadowOffsetX,
                                            float shadowOffsetY,
                                            float strokeSize) {
    CCSize size = CCSizeZero;
	do {
		CC_BREAK_IF(!pText);
        
		CLBitmapDC &dc = sharedCLBitmapDC();
        
		CC_BREAK_IF(!dc.getBitmapFromJavaShadowStroke(pText,
                                                      maxWidth,
                                                      0,
                                                      kAlignCenter,
                                                      pFontName,
                                                      nSize,
                                                      0,
                                                      0,
                                                      0,
                                                      shadowOffsetX != 0 || shadowOffsetY != 0,
                                                      shadowOffsetX,
                                                      shadowOffsetY,
                                                      0,
                                                      0,
                                                      strokeSize != 0,
                                                      0,
                                                      0,
                                                      0,
                                                      strokeSize,
                                                      true));
		size.width = dc.m_nWidth;
		size.height = dc.m_nHeight;
	} while(0);
    
	return size;
}

NS_CC_END

using namespace cocos2d;

extern "C" {
    JNIEXPORT void JNICALL Java_org_cocos2dx_lib_RichLabelBitmap_nativeInitBitmapDC(JNIEnv* env, jclass clazz, jint width, jint height, jbyteArray pixels) {
        CLBitmapDC& bitmapDC = sharedCLBitmapDC();
        bitmapDC.m_nWidth = width;
        bitmapDC.m_nHeight = height;
        
        if(pixels) {
            int size = width * height * 4;
            bitmapDC.m_pData = new unsigned char[size];
            env->GetByteArrayRegion(pixels, 0, size, (jbyte*)bitmapDC.m_pData);
            
            // swap data
            unsigned int* tempPtr = (unsigned int*)bitmapDC.m_pData;
            unsigned int tempdata = 0;
            for (int i = 0; i < height; ++i) {
                for (int j = 0; j < width; ++j) {
                    tempdata = *tempPtr;
                    *tempPtr++ = bitmapDC.swapAlpha(tempdata);
                }
            }
        }
    }

    JNIEXPORT void JNICALL Java_org_cocos2dx_lib_RichLabelBitmap_nativeSaveLinkMeta(JNIEnv* env, jclass clazz, jint normalBgColor, jint selectedBgColor,
			jfloat x, jfloat y, jfloat width, jfloat height, jint tag) {
    	CLBitmapDC& bitmapDC = sharedCLBitmapDC();
    	LinkMeta meta = {
    		normalBgColor,
    		selectedBgColor,
    		tag,
    		x,
    		y,
    		width,
    		height
    	};
    	bitmapDC.m_linkMetas.push_back(meta);
    }

    JNIEXPORT void JNICALL Java_org_cocos2dx_lib_RichLabelBitmap_nativeSaveShadowStrokePadding(JNIEnv* env, jclass clazz, jfloat x, jfloat y) {
    	CLBitmapDC& bitmapDC = sharedCLBitmapDC();
    	bitmapDC.m_shadowStrokePadding.x = x;
    	bitmapDC.m_shadowStrokePadding.y = y;
    }

    JNIEXPORT void JNICALL Java_org_cocos2dx_lib_RichLabelBitmap_nativeResetBitmapDC(JNIEnv* env, jclass clazz) {
    	CLBitmapDC& bitmapDC = sharedCLBitmapDC();
    	bitmapDC.m_linkMetas.clear();
    	bitmapDC.m_shadowStrokePadding = CCPointZero;
    }
    
    JNIEXPORT jstring JNICALL Java_org_cocos2dx_lib_RichLabelBitmap_nativeFullPathForFilename(JNIEnv* env, jclass clazz, jstring filename) {
    	string fn = JniHelper::jstring2string(filename);
    	string path = CCFileUtils::sharedFileUtils()->fullPathForFilename(fn.c_str());
    	return env->NewStringUTF(path.c_str());
    }
}

#endif // #if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
