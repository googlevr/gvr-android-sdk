/*
 * Copyright 2014 Google Inc. All Rights Reserved.

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.vrtoolkit.cardboard.samples.treasurehunt;

import android.content.Context;
import android.graphics.Color;
import android.graphics.Typeface;
import android.util.AttributeSet;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

/**
 * Contains two sub-views to provide a simple stereo HUD.
 */
public class CardboardOverlayView extends LinearLayout {
  private final CardboardOverlayEyeView leftView;
  private final CardboardOverlayEyeView rightView;
  private AlphaAnimation textFadeAnimation;

  public CardboardOverlayView(Context context, AttributeSet attrs) {
    super(context, attrs);
    setOrientation(HORIZONTAL);

    LayoutParams params = new LayoutParams(
      LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT, 1.0f);
    params.setMargins(0, 0, 0, 0);

    leftView = new CardboardOverlayEyeView(context, attrs);
    leftView.setLayoutParams(params);
    addView(leftView);

    rightView = new CardboardOverlayEyeView(context, attrs);
    rightView.setLayoutParams(params);
    addView(rightView);

    // Set some reasonable defaults.
    setDepthOffset(0.01f);
    setColor(Color.rgb(150, 255, 180));
    setVisibility(View.VISIBLE);

    textFadeAnimation = new AlphaAnimation(1.0f, 0.0f);
    textFadeAnimation.setDuration(5000);
  }

  public void show3DToast(String message) {
    setText(message);
    setTextAlpha(1f);
    textFadeAnimation.setAnimationListener(new EndAnimationListener() {
      @Override
      public void onAnimationEnd(Animation animation) {
        setTextAlpha(0f);
      }
    });
    startAnimation(textFadeAnimation);
  }

  private abstract class EndAnimationListener implements Animation.AnimationListener {
    @Override public void onAnimationRepeat(Animation animation) {}
    @Override public void onAnimationStart(Animation animation) {}
  }

  private void setDepthOffset(float offset) {
    leftView.setOffset(offset);
    rightView.setOffset(-offset);
  }

  private void setText(String text) {
    leftView.setText(text);
    rightView.setText(text);
  }

  private void setTextAlpha(float alpha) {
    leftView.setTextViewAlpha(alpha);
    rightView.setTextViewAlpha(alpha);
  }

  private void setColor(int color) {
    leftView.setColor(color);
    rightView.setColor(color);
  }

  /**
   * A simple view group containing some horizontally centered text underneath a horizontally
   * centered image.
   *
   * <p>This is a helper class for CardboardOverlayView.
   */
  private class CardboardOverlayEyeView extends ViewGroup {
    private final ImageView imageView;
    private final TextView textView;
    private float offset;

    public CardboardOverlayEyeView(Context context, AttributeSet attrs) {
      super(context, attrs);
      imageView = new ImageView(context, attrs);
      imageView.setScaleType(ImageView.ScaleType.CENTER_INSIDE);
      imageView.setAdjustViewBounds(true);  // Preserve aspect ratio.
      addView(imageView);

      textView = new TextView(context, attrs);
      textView.setTextSize(TypedValue.COMPLEX_UNIT_DIP, 14.0f);
      textView.setTypeface(textView.getTypeface(), Typeface.BOLD);
      textView.setGravity(Gravity.CENTER);
      textView.setShadowLayer(3.0f, 0.0f, 0.0f, Color.DKGRAY);
      addView(textView);
    }

    public void setColor(int color) {
      imageView.setColorFilter(color);
      textView.setTextColor(color);
    }

    public void setText(String text) {
      textView.setText(text);
    }

    public void setTextViewAlpha(float alpha) {
      textView.setAlpha(alpha);
    }

    public void setOffset(float offset) {
      this.offset = offset;
    }

    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
      // Width and height of this ViewGroup.
      final int width = right - left;
      final int height = bottom - top;

      // The size of the image, given as a fraction of the dimension as a ViewGroup.
      // We multiply both width and heading with this number to compute the image's bounding
      // box. Inside the box, the image is the horizontally and vertically centered.
      final float imageSize = 0.1f;

      // The fraction of this ViewGroup's height by which we shift the image off the
      // ViewGroup's center. Positive values shift downwards, negative values shift upwards.
      final float verticalImageOffset = -0.07f;

      // Vertical position of the text, specified in fractions of this ViewGroup's height.
      final float verticalTextPos = 0.52f;

      // Layout ImageView
      float adjustedOffset = offset;
      // If the half screen width is bigger than 1000 pixels, that means it's a big screen
      // phone and we need to use a different offset value.
      if (width > 1000) {
        adjustedOffset = 3.8f * offset;
      }
      float imageMargin = (1.0f - imageSize) / 2.0f;
      float leftMargin = (int) (width * (imageMargin + adjustedOffset));
      float topMargin = (int) (height * (imageMargin + verticalImageOffset));
      imageView.layout(
        (int) leftMargin, (int) topMargin,
        (int) (leftMargin + width * imageSize), (int) (topMargin + height * imageSize));

      // Layout TextView
      leftMargin = adjustedOffset * width;
      topMargin = height * verticalTextPos;
      textView.layout(
        (int) leftMargin, (int) topMargin,
        (int) (leftMargin + width), (int) (topMargin + height * (1.0f - verticalTextPos)));
    }
  }
}
