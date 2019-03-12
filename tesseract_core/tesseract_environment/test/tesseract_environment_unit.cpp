#include <tesseract_environment/core/macros.h>
TESSERACT_ENVIRONMENT_IGNORE_WARNINGS_PUSH
#include <ros/package.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <tesseract_scene_graph/parser/urdf_parser.h>
#include <tesseract_collision/bullet/bullet_discrete_bvh_manager.h>
#include <tesseract_collision/bullet/bullet_cast_bvh_manager.h>
TESSERACT_ENVIRONMENT_IGNORE_WARNINGS_POP

#include <tesseract_environment/core/types.h>
#include "tesseract_environment/kdl/kdl_env.h"

using namespace tesseract_scene_graph;
using namespace tesseract_collision;
using namespace tesseract_environment;

std::string locateResource(const std::string& url)
{
  std::string mod_url = url;
  if (url.find("package://") == 0)
  {
    mod_url.erase(0, strlen("package://"));
    size_t pos = mod_url.find("/");
    if (pos == std::string::npos)
    {
      return std::string();
    }

    std::string package = mod_url.substr(0, pos);
    mod_url.erase(0, pos);
    std::string package_path = ros::package::getPath(package);

    if (package_path.empty())
    {
      return std::string();
    }

    mod_url = package_path + mod_url; // "file://" + package_path + mod_url;
  }

  return mod_url;
}

SceneGraphPtr getSceneGraph()
{
  std::string path = std::string(DATA_DIR) + "/urdf/lbr_iiwa_14_r820.urdf";

  tesseract_scene_graph::ResourceLocatorFn locator = locateResource;
  return tesseract_scene_graph::parseURDF(path, locator);
}

void runContactManagerCloneTest(const tesseract_environment::EnvironmentPtr& env)
{
  // Test after clone if active list correct
  tesseract_collision::DiscreteContactManagerPtr discrete_manager = env->getDiscreteContactManager();
  const std::vector<std::string>& e_active_list = env->getActiveLinkNames();
  const std::vector<std::string>& d_active_list = discrete_manager->getActiveCollisionObjects();
  EXPECT_TRUE(std::equal(e_active_list.begin(), e_active_list.end(), d_active_list.begin()));

  tesseract_collision::ContinuousContactManagerPtr cast_manager = env->getContinuousContactManager();
  const std::vector<std::string>& c_active_list = cast_manager->getActiveCollisionObjects();
  EXPECT_TRUE(std::equal(e_active_list.begin(), e_active_list.end(), c_active_list.begin()));
}

void runAddandRemoveLinkTest(const tesseract_environment::EnvironmentPtr& env)
{
  LinkPtr link_1(new Link("link_n1"));
  LinkPtr link_2(new Link("link_n2"));

  JointPtr joint_1(new Joint("joint_n1"));
  joint_1->parent_to_joint_origin_transform.translation()(0) = 1.25;
  joint_1->parent_link_name = "link_n1";
  joint_1->child_link_name = "link_n2";
  joint_1->type = JointType::FIXED;

  env->addLink(link_1);

  std::vector<std::string> link_names = env->getLinkNames();
  std::vector<std::string> joint_names = env->getJointNames();
  EXPECT_TRUE(std::find(link_names.begin(), link_names.end(), link_1->getName()) != link_names.end());
  EXPECT_TRUE(std::find(joint_names.begin(), joint_names.end(), "joint_" + link_1->getName()) != joint_names.end());

  env->addLink(link_2, joint_1);
  link_names = env->getLinkNames();
  joint_names = env->getJointNames();
  EXPECT_TRUE(std::find(link_names.begin(), link_names.end(), link_2->getName()) != link_names.end());
  EXPECT_TRUE(std::find(joint_names.begin(), joint_names.end(), joint_1->getName()) != joint_names.end());

  env->getSceneGraph()->saveDOT("/tmp/before_remove_link_unit.dot");

  env->removeLink(link_1->getName());
  link_names = env->getLinkNames();
  joint_names = env->getJointNames();
  EXPECT_TRUE(std::find(link_names.begin(), link_names.end(), link_1->getName()) == link_names.end());
  EXPECT_TRUE(std::find(joint_names.begin(), joint_names.end(), "joint_" + link_1->getName()) == joint_names.end());
  EXPECT_TRUE(std::find(link_names.begin(), link_names.end(), link_2->getName()) == link_names.end());
  EXPECT_TRUE(std::find(joint_names.begin(), joint_names.end(), joint_1->getName()) == joint_names.end());

  env->getSceneGraph()->saveDOT("/tmp/after_remove_link_unit.dot");
}

void runMoveLinkandJointTest(const tesseract_environment::EnvironmentPtr& env)
{
  LinkPtr link_1(new Link("link_n1"));
  LinkPtr link_2(new Link("link_n2"));

  JointPtr joint_1(new Joint("joint_n1"));
  joint_1->parent_link_name = env->getRootLinkName();
  joint_1->child_link_name = "link_n1";
  joint_1->type = JointType::FIXED;

  JointPtr joint_2(new Joint("joint_n2"));
  joint_2->parent_to_joint_origin_transform.translation()(0) = 1.25;
  joint_2->parent_link_name = "link_n1";
  joint_2->child_link_name = "link_n2";
  joint_2->type = JointType::FIXED;

  env->addLink(link_1, joint_1);
  env->addLink(link_2, joint_2);
  std::vector<std::string> link_names = env->getLinkNames();
  std::vector<std::string> joint_names = env->getJointNames();
  EXPECT_TRUE(std::find(link_names.begin(), link_names.end(), link_1->getName()) != link_names.end());
  EXPECT_TRUE(std::find(link_names.begin(), link_names.end(), link_2->getName()) != link_names.end());
  EXPECT_TRUE(std::find(joint_names.begin(), joint_names.end(), joint_1->getName()) != joint_names.end());
  EXPECT_TRUE(std::find(joint_names.begin(), joint_names.end(), joint_2->getName()) != joint_names.end());

  env->getSceneGraph()->saveDOT("/tmp/before_move_joint_unit.dot");

  env->moveJoint("joint_n1", "tool0");
  link_names = env->getLinkNames();
  joint_names = env->getJointNames();
  EXPECT_TRUE(env->getJoint("joint_n1")->parent_link_name == "tool0");
  EXPECT_TRUE(std::find(link_names.begin(), link_names.end(), link_1->getName()) != link_names.end());
  EXPECT_TRUE(std::find(link_names.begin(), link_names.end(), link_2->getName()) != link_names.end());
  EXPECT_TRUE(std::find(joint_names.begin(), joint_names.end(), joint_1->getName()) != joint_names.end());
  EXPECT_TRUE(std::find(joint_names.begin(), joint_names.end(), joint_2->getName()) != joint_names.end());

  env->getSceneGraph()->saveDOT("/tmp/after_move_joint_unit.dot");
}

/// Testing AllowedCollisionMatrix
TEST(TesseractEnvironmentUnit, TestAllowedCollisionMatrix)
{
  tesseract_environment::AllowedCollisionMatrix acm;

  acm.addAllowedCollision("link1", "link2", "test");
  // collision between link1 and link2 should be allowed
  EXPECT_TRUE(acm.isCollisionAllowed("link1", "link2"));
  // but now between link2 and link3
  EXPECT_FALSE(acm.isCollisionAllowed("link2", "link3"));

  acm.removeAllowedCollision("link1", "link2");
  // now collision link1 and link2 is not allowed anymore
  EXPECT_FALSE(acm.isCollisionAllowed("link1", "link2"));

  acm.addAllowedCollision("link3", "link3", "test");
  EXPECT_EQ(acm.getAllAllowedCollisions().size(), 1);
  acm.clearAllowedCollisions();
  EXPECT_EQ(acm.getAllAllowedCollisions().size(), 0);
}

TEST(TesseractEnvironmentUnit, KDLEnvCloneContactManagerUnit)
{
  tesseract_scene_graph::SceneGraphPtr scene_graph = getSceneGraph();
  EXPECT_TRUE(scene_graph != nullptr);

  tesseract_environment::KDLEnvPtr env(new tesseract_environment::KDLEnv);
  EXPECT_TRUE(env != nullptr);

  bool success = env->init(scene_graph);
  env->setDiscreteContactManager(tesseract_collision_bullet::BulletDiscreteBVHManagerPtr(new tesseract_collision_bullet::BulletDiscreteBVHManager()));
  env->setContinuousContactManager(tesseract_collision_bullet::BulletCastBVHManagerPtr(new tesseract_collision_bullet::BulletCastBVHManager()));
  EXPECT_TRUE(success);

  runContactManagerCloneTest(env);
}

TEST(TesseractEnvironmentUnit, KDLEnvAddandRemoveLink)
{
  SceneGraphPtr scene_graph = getSceneGraph();
  EXPECT_TRUE(scene_graph != nullptr);

  KDLEnvPtr env(new KDLEnv());
  EXPECT_TRUE(env != nullptr);

  bool success = env->init(scene_graph);
  env->setDiscreteContactManager(tesseract_collision_bullet::BulletDiscreteBVHManagerPtr(new tesseract_collision_bullet::BulletDiscreteBVHManager()));
  env->setContinuousContactManager(tesseract_collision_bullet::BulletCastBVHManagerPtr(new tesseract_collision_bullet::BulletCastBVHManager()));
  EXPECT_TRUE(success);

  runAddandRemoveLinkTest(env);
}

TEST(TesseractEnvironmentUnit, KDLEnvMoveLinkandJoint)
{
  SceneGraphPtr scene_graph = getSceneGraph();
  assert(scene_graph != nullptr);

  KDLEnvPtr env(new KDLEnv());
  assert(env != nullptr);

  bool success = env->init(scene_graph);
  env->setDiscreteContactManager(tesseract_collision_bullet::BulletDiscreteBVHManagerPtr(new tesseract_collision_bullet::BulletDiscreteBVHManager()));
  env->setContinuousContactManager(tesseract_collision_bullet::BulletCastBVHManagerPtr(new tesseract_collision_bullet::BulletCastBVHManager()));
  assert(success);

  runMoveLinkandJointTest(env);
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
