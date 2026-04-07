
#include "Character.h"

/**
 * @brief Character 构造函数
 */
Character::Character()
{
    // 初始化 CharacterAppearance 结构体中的默认值
    m_appearance.age = 0;
}

// 注册元类型
static const int s_characterMetaType = qRegisterMetaType<Character>();
static const int s_appearanceMetaType = qRegisterMetaType<CharacterAppearance>();
static const int s_configMetaType = qRegisterMetaType<CharacterConfiguration>();
