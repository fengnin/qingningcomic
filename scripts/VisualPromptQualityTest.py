#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
VisualPrompt质量测试工具 (Python版本)

这个脚本用于测试Qwen生成的visualPrompt是否符合规范
可以直接运行，不需要编译

使用方法：
    python VisualPromptQualityTest.py

作者：青柠漫画项目
日期：2026-05-08
"""

from typing import List, Dict, Tuple
from dataclasses import dataclass


@dataclass
class TestResult:
    """测试结果"""
    passed: bool
    issue: str = ""
    suggestion: str = ""


@dataclass
class TestCase:
    """测试用例"""
    name: str
    visual_prompt: str
    expected_scene_position: str = ""
    expected_character_position: str = ""
    has_dialogue: bool = False


class VisualPromptQualityChecker:
    """VisualPrompt质量检查器"""
    
    @staticmethod
    def check_visual_prompt(visual_prompt: str, test_case: TestCase) -> List[TestResult]:
        """检查visualPrompt质量"""
        results = []
        
        results.append(VisualPromptQualityChecker.check_scene_position(
            visual_prompt, test_case.expected_scene_position))
        results.append(VisualPromptQualityChecker.check_character_position(
            visual_prompt, test_case.expected_character_position))
        results.append(VisualPromptQualityChecker.check_dialogue_element(
            visual_prompt, test_case.has_dialogue))
        results.append(VisualPromptQualityChecker.check_lighting_description(visual_prompt))
        results.append(VisualPromptQualityChecker.check_composition_language(visual_prompt))
        results.append(VisualPromptQualityChecker.check_literary_expressions(visual_prompt))
        
        return results
    
    @staticmethod
    def check_scene_position(visual_prompt: str, expected: str) -> TestResult:
        """检查场景位置描述"""
        vague_words = ["门口", "窗边", "室内", "室外"]
        
        for word in vague_words:
            if word in visual_prompt:
                return TestResult(
                    passed=False,
                    issue=f"场景位置描述模糊：'{word}'",
                    suggestion="建议改为：'从店内朝向玻璃门方向' 或 'sitting behind counter facing door'"
                )
        
        good_keywords = ["朝向", "视角", "方向", "from", "facing"]
        has_good_keyword = any(keyword in visual_prompt for keyword in good_keywords)
        
        if expected and not has_good_keyword and expected not in visual_prompt:
            return TestResult(
                passed=False,
                issue="缺少明确的场景位置描述",
                suggestion=f"建议添加：'{expected}'"
            )
        
        return TestResult(passed=True)
    
    @staticmethod
    def check_character_position(visual_prompt: str, expected: str) -> TestResult:
        """检查人物位置描述"""
        vague_expressions = ["在画面中央", "相对而立", "视线交汇", "目光交汇"]
        
        for expr in vague_expressions:
            if expr in visual_prompt:
                return TestResult(
                    passed=False,
                    issue=f"人物位置描述抽象：'{expr}'",
                    suggestion="建议改为：'character A on left side, character B on right side'"
                )
        
        if expected and expected not in visual_prompt:
            return TestResult(
                passed=False,
                issue="缺少明确的人物位置描述",
                suggestion=f"建议添加：'{expected}'"
            )
        
        return TestResult(passed=True)
    
    @staticmethod
    def check_dialogue_element(visual_prompt: str, has_dialogue: bool) -> TestResult:
        """检查对话元素"""
        if has_dialogue:
            if "speech bubble" not in visual_prompt and \
               "气泡" not in visual_prompt and \
               "对话" not in visual_prompt:
                return TestResult(
                    passed=False,
                    issue="对话场景缺少气泡元素",
                    suggestion="建议添加：'speech bubble with text: \"xxx\"'"
                )
        
        return TestResult(passed=True)
    
    @staticmethod
    def check_lighting_description(visual_prompt: str) -> TestResult:
        """检查光影描述"""
        vague_lighting = ["柔光", "逆光", "光影交错", "光漫溢"]
        
        for light in vague_lighting:
            if light in visual_prompt:
                return TestResult(
                    passed=False,
                    issue=f"光影描述抽象：'{light}'",
                    suggestion="建议改为：'soft light from left window' 或 'natural light from glass door'"
                )
        
        return TestResult(passed=True)
    
    @staticmethod
    def check_composition_language(visual_prompt: str) -> TestResult:
        """检查构图语言"""
        vague_composition = ["氛围温馨", "情感丰富", "意境深远"]
        
        for comp in vague_composition:
            if comp in visual_prompt:
                return TestResult(
                    passed=False,
                    issue=f"构图描述抽象：'{comp}'",
                    suggestion="建议改为：'diagonal composition' 或 'rule-of-thirds'"
                )
        
        return TestResult(passed=True)
    
    @staticmethod
    def check_literary_expressions(visual_prompt: str) -> TestResult:
        """检查文学性表达"""
        literary_expressions = ["立于", "佝偻于", "伫立于", "凝视着"]
        
        for expr in literary_expressions:
            if expr in visual_prompt:
                return TestResult(
                    passed=False,
                    issue=f"使用文学性表达：'{expr}'",
                    suggestion="建议改为：'standing in' 或 'looking at'"
                )
        
        return TestResult(passed=True)


def run_tests():
    """运行测试"""
    print("=" * 60)
    print("VisualPrompt质量测试工具")
    print("=" * 60)
    print()
    
    test_cases = [
        TestCase(
            name="测试用例1：青柠第一章面板1-2（当前版本）",
            visual_prompt=(
                "文艺少女漫画风格，拾光书店门口，铜风铃轻晃，门外绿意朦胧，"
                "白发老人佝偻立于逆光门框中，攥旧布包，眼神急切；"
                "年轻女子坐于柜台后，已放下薄荷糖，微微前倾，目光温静，"
                "两人视线在画面中央交汇，柔光漫溢，空气中有飘浮栀子花瓣"
            ),
            expected_scene_position="从店内朝向玻璃门",
            expected_character_position="on left side",
            has_dialogue=True
        ),
        TestCase(
            name="测试用例2：优化后的版本（预期）",
            visual_prompt=(
                "文艺少女漫画风格，拾光书店内部从柜台视角朝向玻璃门方向，"
                "白发老人（陈伯）standing at entrance, visible from inside, "
                "身穿洗旧藏青布衫，body leaning forward 20 degrees, anxious expression; "
                "年轻女子（青柠）sitting behind counter on left side, "
                "body turning right facing old man, concerned expression; "
                "two characters in L-shaped composition, "
                "speech bubble: \"爷爷，您找什么书？\" said by 青柠; "
                "natural light from glass door creating warm tones"
            ),
            expected_scene_position="从店内朝向玻璃门",
            expected_character_position="on left side",
            has_dialogue=True
        ),
        TestCase(
            name="测试用例3：简单场景（无对话）",
            visual_prompt=(
                "manga style, quiet bookstore interior, "
                "young woman sitting behind counter on left side, "
                "reading a book, soft natural light from window on right"
            ),
            expected_scene_position="interior",
            expected_character_position="on left side",
            has_dialogue=False
        ),
    ]
    
    for i, test_case in enumerate(test_cases, 1):
        print(f"【{test_case.name}】")
        print(f"VisualPrompt: {test_case.visual_prompt[:100]}...")
        print()
        
        results = VisualPromptQualityChecker.check_visual_prompt(
            test_case.visual_prompt, test_case)
        
        passed_count = sum(1 for r in results if r.passed)
        failed_count = sum(1 for r in results if not r.passed)
        
        for result in results:
            if not result.passed:
                print(f"  ❌ 问题：{result.issue}")
                print(f"     建议：{result.suggestion}")
                print()
        
        print(f"测试结果：{passed_count} 通过，{failed_count} 失败")
        print("-" * 60)
        print()
    
    print("测试完成！")
    print()
    print("总结：")
    print("1. 测试用例1（当前版本）应该有多个失败项")
    print("2. 测试用例2（优化版本）应该全部通过")
    print("3. 测试用例3（简单场景）应该全部通过")
    print("4. 如果测试用例2也有失败项，说明测试规则需要调整")


if __name__ == "__main__":
    run_tests()
