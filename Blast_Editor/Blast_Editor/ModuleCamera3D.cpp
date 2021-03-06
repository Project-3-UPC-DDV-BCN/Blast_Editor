#include "Globals.h"
#include "Application.h"
#include "ModuleCamera3D.h"
#include "ComponentCamera.h"
#include "ModuleInput.h"
#include "ModuleRenderer3D.h"
#include "ModuleWindow.h"
#include <vector>
#include "Math/Quat.h"
#include "Math/float3x3.h"
#include "BlastMesh.h"
#include "ModuleBlast.h"

ModuleCamera3D::ModuleCamera3D(Application* app) : Module(app)
{

	name = "Camera";
	can_update = false;
	camera_is_orbital = false;
	camera_sensitivity = 0.25f;

	key_speed = 38;//DEFAULT LSHIFT
	key_forward = 22;//DEFAULT W
	key_backward = 18;//DEFAULT S
	key_up = 16;//DEFAULT Q
	key_down = 4;//DEFAULT E
	key_left = 0;//DEFAULT A
	key_right = 3;//DEFAULT D
}

ModuleCamera3D::~ModuleCamera3D()
{}
// -----------------------------------------------------------------
bool ModuleCamera3D::Init()
{
	CONSOLE_DEBUG("Setting up the camera");
	
	return true;
}


void ModuleCamera3D::CreateEditorCamera()
{
	editor_camera = new ComponentCamera();
	App->renderer3D->editor_camera = editor_camera;
}

// -----------------------------------------------------------------
bool ModuleCamera3D::CleanUp()
{
	CONSOLE_DEBUG("Cleaning camera");
	RELEASE(editor_camera);
	App->renderer3D->editor_camera = nullptr;
	return true;
}

// -----------------------------------------------------------------
update_status ModuleCamera3D::Update(float dt)
{
	ms_timer.Start();
	if (App->input->GetMouseButton(SDL_BUTTON_RIGHT) == KEY_REPEAT)
	{
		// Implement a debug camera with keys and mouse
		// Now we can make this movememnt frame rate independant!
		math::Frustum* tmp_camera_frustum = &editor_camera->camera_frustum;
		float3 new_pos(0, 0, 0);
		float speed = 20.0f * dt;
		if (App->input->GetKey(key_speed) == KEY_REPEAT)
			speed = 70.0f * dt;

		if (App->input->GetKey(SDL_SCANCODE_Q) == KEY_REPEAT) new_pos.y += speed;
		if (App->input->GetKey(SDL_SCANCODE_E) == KEY_REPEAT) new_pos.y -= speed;

		if (App->input->GetKey(SDL_SCANCODE_W) == KEY_REPEAT) new_pos += tmp_camera_frustum->Front() * speed;
		if (App->input->GetKey(SDL_SCANCODE_S) == KEY_REPEAT) new_pos -= tmp_camera_frustum->Front() * speed;
		if (App->input->GetMouseZ() > 0) new_pos += tmp_camera_frustum->Front() * speed;
		if (App->input->GetMouseZ() < 0) new_pos -= tmp_camera_frustum->Front() * speed;

		if (App->input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT) new_pos -= tmp_camera_frustum->WorldRight() * speed;
		if (App->input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT) new_pos += tmp_camera_frustum->WorldRight() * speed;
		if (!new_pos.IsZero())
		{
			tmp_camera_frustum->Translate(new_pos);
		}

		if (App->input->GetKey(SDL_SCANCODE_F) == KEY_DOWN) FocusOnObject(App->blast->current_selected_mesh->box);

		// Mouse motion ----------------

		if ((App->input->GetMouseButton(SDL_BUTTON_RIGHT) == KEY_REPEAT && !camera_is_orbital) || App->input->GetMouseButton(SDL_BUTTON_LEFT) == KEY_REPEAT && camera_is_orbital)
		{
			float dx = -(float)App->input->GetMouseXMotion() * camera_sensitivity * dt;
			float dy = -(float)App->input->GetMouseYMotion() * camera_sensitivity * dt;

			if (dx != 0)
			{
				Quat rotation_x = Quat::RotateY(dx);
				tmp_camera_frustum->SetFront(rotation_x.Mul(tmp_camera_frustum->Front()).Normalized());
				tmp_camera_frustum->SetUp(rotation_x.Mul(tmp_camera_frustum->Up()).Normalized());
			}

			if (dy != 0)
			{
				Quat rotation_y = Quat::RotateAxisAngle(tmp_camera_frustum->WorldRight(), dy);

				float3 new_up = rotation_y.Mul(tmp_camera_frustum->Up()).Normalized();

				if (new_up.y > 0.0f)
				{
					tmp_camera_frustum->SetUp(new_up);
					tmp_camera_frustum->SetFront(rotation_y.Mul(tmp_camera_frustum->Front()).Normalized());
				}
			}
		}
	}
	
	return UPDATE_CONTINUE;
}

// -----------------------------------------------------------------
void ModuleCamera3D::LookAt(const float3 &spot)
{
	float3 direction = spot - editor_camera->camera_frustum.Pos();

	float3x3 matrix = float3x3::LookAt(editor_camera->camera_frustum.Front(), direction.Normalized(), editor_camera->camera_frustum.Up(), float3::unitY);

	editor_camera->camera_frustum.SetFront(matrix.MulDir(editor_camera->camera_frustum.Front()).Normalized());
	editor_camera->camera_frustum.SetUp(matrix.MulDir(editor_camera->camera_frustum.Up()).Normalized());
}

void ModuleCamera3D::OrbitAt(const float3 & spot)
{

}

void ModuleCamera3D::FocusOnObject(AABB& box)
{
	editor_camera->camera_frustum.SetPos({ box.maxPoint.x, box.maxPoint.y + 20, box.maxPoint.z });

	float3 look_at_pos;

	look_at_pos.x = box.CenterPoint().x;
	look_at_pos.y = box.CenterPoint().y;
	look_at_pos.z = box.CenterPoint().z;

	LookAt(look_at_pos);
}

// -----------------------------------------------------------------
float* ModuleCamera3D::GetViewMatrix()
{
	return editor_camera->GetViewMatrix();
}

void ModuleCamera3D::SetOrbital(bool is_orbital)
{
	camera_is_orbital = is_orbital;
}

bool ModuleCamera3D::IsOrbital() const
{
	return camera_is_orbital;
}

math::float3 ModuleCamera3D::GetPosition() const
{
	return editor_camera->camera_frustum.Pos();
}

void ModuleCamera3D::SetPosition(math::float3 position)
{
	editor_camera->camera_frustum.Translate(position);
}

ComponentCamera * ModuleCamera3D::GetCamera() const
{
	return editor_camera;
}

void ModuleCamera3D::SetCameraSensitivity(float sensivity)
{
	camera_sensitivity = sensivity;
}

float ModuleCamera3D::GetCameraSensitivity() const
{
	return camera_sensitivity;
}

void ModuleCamera3D::MousePickRay(int mouse_x, int mouse_y)
{
	//ImVec2 window_size;
	//window_size.x = App->window->GetWidth();
	//window_size.y = App->window->GetHeight();

	//if (mouse_x > 0 && mouse_x < 0 + window_size.x && mouse_y > 0 && mouse_y < 0 + window_size.y)//If mouse is in scene 
	//{
	//	//Ray needs x and y between [-1,1]
	//	float normalized_mouse_x = (((mouse_x - 0) / window_size.x) * 2) - 1;

	//	float normalized_mouse_y = 1 - ((mouse_y - 0) / window_size.y) * 2;


	//	Ray ray = this->GetCamera()->camera_frustum.UnProject(normalized_mouse_x, normalized_mouse_y);
	//	//------------------------------------------------------------------------------

	//	float min_dist = NULL;
	//	GameObject* closest_object = nullptr;

	//	for (std::list<GameObject*>::iterator it = App->scene->scene_gameobjects.begin(); it != App->scene->scene_gameobjects.end(); it++)
	//	{
	//		Ray inv_ray = ray.ReturnTransform((*it)->GetGlobalTransfomMatrix().Inverted()); //Triangle intersection needs the inverted ray

	//		ComponentMeshRenderer* mesh_renderer = (ComponentMeshRenderer*)(*it)->GetComponent(Component::CompMeshRenderer);
	//		if (mesh_renderer != nullptr && mesh_renderer->GetMesh() != nullptr)
	//		{
	//			float dist_near;
	//			float dist_far;
	//			if (ray.Intersects(mesh_renderer->GetMesh()->box, dist_near, dist_far))//Try intersection with AABB, if it intersects, then try with triangles
	//			{
	//				float* mesh_vertices = mesh_renderer->GetMesh()->vertices;
	//				uint* mesh_indices = mesh_renderer->GetMesh()->indices;
	//				for (int i = 0; i < mesh_renderer->GetMesh()->num_indices; i += 3)//Create Triangles
	//				{
	//					Triangle temp;
	//					temp.a.Set(mesh_vertices[(3 * mesh_indices[i])], mesh_vertices[(3 * mesh_indices[i] + 1)], mesh_vertices[(3 * mesh_indices[i] + 2)]);
	//					temp.b.Set(mesh_vertices[(3 * mesh_indices[i + 1])], mesh_vertices[(3 * mesh_indices[i + 1] + 1)], mesh_vertices[(3 * mesh_indices[i + 1] + 2)]);
	//					temp.c.Set(mesh_vertices[(3 * mesh_indices[i + 2])], mesh_vertices[(3 * mesh_indices[i + 2] + 1)], mesh_vertices[(3 * mesh_indices[i + 2] + 2)]);

	//					if (inv_ray.Intersects(temp))//If it intersects, save the distance 
	//					{
	//						if (min_dist == NULL || dist_near < min_dist)
	//						{
	//							min_dist = dist_near;
	//							if (closest_object != nullptr)
	//							{
	//								closest_object->SetSelected(false);
	//							}
	//							closest_object = *it;
	//						}
	//					}
	//				}
	//			}
	//		}
	//	}
	//	if (closest_object != nullptr)
	//	{
	//		App->scene->selected_gameobjects.clear();
	//		for (std::list<GameObject*>::iterator it = App->scene->scene_gameobjects.begin(); it != App->scene->scene_gameobjects.end(); it++)
	//		{
	//			(*it)->SetSelected(false);
	//		}
	//		closest_object->SetSelected(true);
	//		App->scene->selected_gameobjects.push_back(closest_object);
	//	}
	//	else//If clicks but doesn't intersect an object, remove selected objects
	//	{
	//		App->scene->selected_gameobjects.clear();
	//		for (std::list<GameObject*>::iterator it = App->scene->scene_gameobjects.begin(); it != App->scene->scene_gameobjects.end(); it++)
	//		{
	//			(*it)->SetSelected(false);
	//		}
	//	}
	//}
}