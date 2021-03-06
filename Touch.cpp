//
// Copyright (c) 2008-2014 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "BorderImage.h"
#include "Camera.h"
#include "Character.h"
#include "Controls.h"
#include "Drawable.h"
#include "Graphics.h"
#include "Input.h"
#include "InputEvents.h"
#include "Log.h"
#include "Octree.h"
#include "PhysicsWorld.h"
#include "Renderer.h"
#include "ResourceCache.h"
#include "RigidBody.h"
#include "Scene.h"
#include "Texture2D.h"
#include "Touch.h"
#include "UI.h"

Touch::Touch(Context* context) :
    Object(context),
    cameraDistance_(CAMERA_INITIAL_DIST),
    touchButtonSize_(192),
    touchButtonBorder_(72),
    /*moveTouchID_(-1),
    rotateTouchID_(-1),
    fireTouchID_(-1),*/
    firstPerson_(false),
    newFirstPerson_(false),
    shadowMode_(false),
    zoom_(false),
    touchEnabled_(false),
	touchMoved_(false),
	maxMovement_(0),
	deltaXY_(IntVector2::ZERO)
{
}

Touch::~Touch()
{
}

void Touch::InitTouchInput()
{
    UI* ui = GetSubsystem<UI>();
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    /*moveButton_ = ui->GetRoot()->CreateChild<BorderImage>();
    moveButton_->SetTexture(cache->GetResource<Texture2D>("Textures/TouchInput.png"));
    moveButton_->SetImageRect(IntRect(0, 0, 96, 96));
    moveButton_->SetAlignment(HA_LEFT, VA_CENTER);
    moveButton_->SetPosition(touchButtonBorder_, -touchButtonBorder_);
    moveButton_->SetSize(touchButtonSize_, touchButtonSize_);
    moveButton_->SetOpacity(0.25f);*/

    /*fireButton_ = ui->GetRoot()->CreateChild<BorderImage>();
    fireButton_->SetTexture(cache->GetResource<Texture2D>("Textures/TouchInput.png"));
    fireButton_->SetImageRect(IntRect(96, 0, 192, 96));
    fireButton_->SetAlignment(HA_RIGHT, VA_BOTTOM);
    fireButton_->SetPosition(-touchButtonBorder_, -touchButtonBorder_);
    fireButton_->SetSize(touchButtonSize_, touchButtonSize_);
    fireButton_->SetOpacity(0.25f);*/
    
    touchEnabled_ = true;
}

void Touch::SubscribeToTouchEvents()
{
	SubscribeToEvent(E_TOUCHBEGIN, HANDLER(Touch, HandleTouchBegin));
	SubscribeToEvent(E_TOUCHEND, HANDLER(Touch, HandleTouchEnd));
	SubscribeToEvent(E_TOUCHMOVE, HANDLER(Touch, HandleTouchMove));
}

void Touch::UpdateTouches(Controls& controls) // Called from HandleUpdate
{
    if (!scene_ || !cameraNode_)
        return;

    Camera* camera = cameraNode_->GetComponent<Camera>();
    if (!camera)
        return;
    
    zoom_ = false; // reset bool

    Graphics* graphics = GetSubsystem<Graphics>();
    Input* input = GetSubsystem<Input>();

    // Touch Inputs
    if (touchEnabled_)
    {
        // Zoom in/out
        if (input->GetNumTouches() == 2)
        {
            TouchState* touch1 = input->GetTouch(0);
            TouchState* touch2 = input->GetTouch(1);

            // Check for zoom pattern (touches moving in opposite directions)
            if ((touch1->delta_.y_ > 0 && touch2->delta_.y_ < 0) || (touch1->delta_.y_ < 0 && touch2->delta_.y_ > 0)) 
                zoom_ = true;
            else 
                zoom_ = false;

            if (zoom_)
            {
                int sens = 0;
                // Check for zoom direction (in/out)
                if (Abs(touch1->position_.y_ - touch2->position_.y_) > Abs(touch1->lastPosition_.y_ - touch2->lastPosition_.y_))
                    sens = -1;
                else
                    sens = 1;
                cameraDistance_ += Abs(touch1->delta_.y_ - touch2->delta_.y_) * sens * TOUCH_SENSITIVITY / 50.0f;
                cameraDistance_ = Clamp(cameraDistance_, CAMERA_MIN_DIST, CAMERA_MAX_DIST); // Restrict zoom range to [1;20]
            }
        }

        // Switch 1st/3rd person mode
        if (input->GetNumTouches() == 3)
            newFirstPerson_ = !firstPerson_;

        // Switch draw debug
        if (input->GetNumTouches() == 4)
            shadowMode_ = !GetSubsystem<Renderer>()->GetDrawShadows();

        // Rotate and Move
        if (!zoom_)
		{
			controls.Set(CTRL_FORWARD, true);
            for (unsigned i = 0; i < input->GetNumTouches(); ++i)
            {
                /*TouchState* touch = input->GetTouch(i);

                if (touch->touchID_ == rotateTouchID_)
                {
                    controls.yaw_ += TOUCH_SENSITIVITY * camera->GetFov() / (float)graphics->GetHeight() * touch->delta_.x_;
                    controls.pitch_ += TOUCH_SENSITIVITY * camera->GetFov() / (float)graphics->GetHeight() * touch->delta_.y_;
                    controls.pitch_ = Clamp(controls.pitch_, -80.0f, 80.0f); // Limit pitch
                }*/

				if (touchMoved_ && deltaXY_ != IntVector2::ZERO)
				{
					float degree = Urho3D::Atan2(-(float)deltaXY_.y_, (float)deltaXY_.x_);
					if (IsLeft(deltaXY_, degree))
					{
						controls.Set(CTRL_LEFT, true);
						maxMovement_++;
					}

					if (IsRight(deltaXY_, degree))
					{
						controls.Set(CTRL_RIGHT, true);
						maxMovement_++;
					}

					if (IsBack(deltaXY_, degree))
					{
						controls.Set(CTRL_BACK, true);
						maxMovement_++;
					}

					if (IsUp(deltaXY_, degree))
					{
						controls.Set(CTRL_JUMP, true);
						maxMovement_++;
					}
				}

                /*if (touch->touchID_ == moveTouchID_)
                {
                    int relX = touch->position_.x_ - moveButton_->GetScreenPosition().x_ - touchButtonSize_ / 2;
                    int relY = touch->position_.y_ - moveButton_->GetScreenPosition().y_ - touchButtonSize_ / 2;
                    if (relY < 0 && Abs(relX * 3 / 2) < Abs(relY))
                        controls.Set(CTRL_JUMP, true);
                    if (relY > 0 && Abs(relX * 3 / 2) < Abs(relY))
                        controls.Set(CTRL_BACK, true);
                    if (relX < 0 && Abs(relY * 3 / 2) < Abs(relX))
                        controls.Set(CTRL_LEFT, true);
                    if (relX > 0 && Abs(relY * 3 / 2) < Abs(relX))
                        controls.Set(CTRL_RIGHT, true);
                }*/
            }
        }

        /*if (fireTouchID_ >= 0)
            controls.Set(CTRL_JUMP, true);*/
    }

    // Gyroscope (emulated by SDL through a virtual joystick)
    /*if (input->GetNumJoysticks() > 0) // numJoysticks = 1 on iOS & Android
    {
        JoystickState* joystick = input->GetJoystick(0);
        if (joystick->GetNumAxes() >= 2)
		{
			if (joystick->GetAxisPosition(1) < -GYROSCOPE_THRESHOLD)
				controls.Set(CTRL_FORWARD, true);
            if (joystick->GetAxisPosition(0) < -GYROSCOPE_THRESHOLD)
                controls.Set(CTRL_LEFT, true);
            if (joystick->GetAxisPosition(0) > GYROSCOPE_THRESHOLD)
                controls.Set(CTRL_RIGHT, true);
            if (joystick->GetAxisPosition(1) > GYROSCOPE_THRESHOLD)
                controls.Set(CTRL_BACK, true);
        }
    }*/
}

void Touch::HandleTouchBegin(StringHash eventType, VariantMap& eventData)
{
    using namespace TouchBegin;

	touchMoved_ = true;
	deltaXY_ = IntVector2::ZERO;
	maxMovement_ = 0;

    /*int touchID = eventData[P_TOUCHID].GetInt(); // Get #touches or dragging value
    IntVector2 pos(eventData[P_X].GetInt(), eventData[P_Y].GetInt()); // Get touch coordinates

    UI* ui = GetSubsystem<UI>();
    UIElement* element = ui->GetElementAt(pos, false); // Get gamepad UIElement touched (if any)

	// Check for gamepad button touched. If none, rotate
	if (element == moveButton_)
		moveTouchID_ = touchID;
	if (element == fireButton_)
		fireTouchID_ = touchID;
    else
        rotateTouchID_ = touchID;*/

    // Init Raycast (for example to acquire a target)
    /*if (!cameraNode_ || !scene_)
        return;
    Camera* camera = cameraNode_->GetComponent<Camera>();
    if (!camera)
        return;

    Graphics* graphics = GetSubsystem<Graphics>();
    Ray cameraRay = camera->GetScreenRay((float)pos.x_ / (float)graphics->GetWidth(), (float)pos.y_ / (float)graphics->GetHeight());

    // Raycast of RigidBodies
    PhysicsRaycastResult result;
    scene_->GetComponent<PhysicsWorld>()->RaycastSingle(result, cameraRay, camera->GetFarClip(), 2); // NB: here we restrict targets to layer 2
    if (result.body_)
        LOGINFO("Physics raycast hit " + result.body_->GetNode()->GetName());

    // Raycast of drawable components (for targets without physics)
    PODVector<RayQueryResult> result2;
    RayOctreeQuery rayQuery(result2, cameraRay, RAY_TRIANGLE, camera->GetFarClip(), DRAWABLE_GEOMETRY);
    scene_->GetComponent<Octree>()->RaycastSingle(rayQuery);
    if (result2.Size())
        LOGINFO("Drawable raycast hit " + result2[0].drawable_->GetNode()->GetName());*/
}

void Touch::HandleTouchEnd(StringHash eventType, VariantMap& eventData)
{
    using namespace TouchBegin;

	//int touchID = eventData[P_TOUCHID].GetInt();
	//IntVector2 pos(eventData[P_X].GetInt(), eventData[P_Y].GetInt()); // Get touch coordinates

	touchMoved_ = false;

    /*if (touchID == moveTouchID_)
        moveTouchID_ = -1;
    if (touchID == rotateTouchID_)
        rotateTouchID_ = -1;
    if (touchID == fireTouchID_)
        fireTouchID_ = -1;*/

    // On-release Update
    firstPerson_ = newFirstPerson_;
    GetSubsystem<Renderer>()->SetDrawShadows(shadowMode_);
}

void Touch::HandleTouchMove(StringHash eventType, VariantMap& eventData)
{
	using namespace TouchMove;

	IntVector2 pos(eventData[P_X].GetInt(), eventData[P_Y].GetInt()); // Get touch coordinates
	IntVector2 deltaPos(eventData[P_DX].GetInt(), eventData[P_DY].GetInt()); // Get touch coordinates
	unsigned int sampleSize = 3;

	if(moveTouches_.Size() >= sampleSize)
	{
		for (unsigned int i = 0; i < moveTouches_.Size(); i++)
			deltaXY_ += moveTouches_[i];

		if (maxMovement_ > 1)
			deltaXY_ = IntVector2::ZERO;
		else
			deltaXY_ /= sampleSize;

		moveTouches_.Clear();
	}
	else
	{
		moveTouches_.Push(deltaPos);
	}
}

void Touch::Reset()
{
	touchMoved_ = false;
	maxMovement_ = 0;
	deltaXY_ = IntVector2::ZERO;
}

bool Touch::IsBack(const IntVector2& delta, float degree)
{
	if (Urho3D::Equals(degree, -90.0f) || (degree < -45 && degree > -90) || (degree < -90 && degree > -135))
		return true;

	return false;
}

bool Touch::IsLeft(const IntVector2& delta, float degree)
{
	if (Urho3D::Equals(degree, -180.0f) || (degree < 180 && degree > 135) || (degree > -180 && degree < -135))
		return true;

	return false;
}

bool Touch::IsRight(const IntVector2& delta, float degree)
{
	if (Urho3D::Equals(degree, 0.0f) || (degree < 45 && degree > 0) || (degree < 0 && degree > -45))
		return true;

	return false;
}

bool Touch::IsUp(const IntVector2& delta, float degree)
{
	if (Urho3D::Equals(degree, 90.0f) || (degree < 90 && degree > 45) || (degree < 135 && degree > 90))
		return true;

	return false;
}
