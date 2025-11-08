#!/usr/bin/env python3
"""
.vcxproj.filters 파일을 업데이트하여 모든 .generated.cpp 파일을
Generate 가상 폴더에 넣습니다.
"""

import re
from pathlib import Path
import sys


def update_filters_file(filters_path: Path):
    """
    .vcxproj.filters 파일을 읽어서 .generated.cpp 파일들에
    <Filter>Generate</Filter>를 추가합니다.
    """
    if not filters_path.exists():
        print(f"❌ Error: File not found: {filters_path}")
        return False

    content = filters_path.read_text(encoding='utf-8')
    original_content = content

    # .generated.cpp 파일을 찾아서 Filter 태그 추가
    # 패턴: <ClCompile Include="Generated\XXX.generated.cpp" />
    # 변경: <ClCompile Include="Generated\XXX.generated.cpp">
    #         <Filter>Generate</Filter>
    #       </ClCompile>

    def add_filter(match):
        filepath = match.group(1)
        # 이미 Filter가 있는지 확인
        if '<Filter>' in match.group(0):
            return match.group(0)

        return f'''    <ClCompile Include="{filepath}">
      <Filter>Generate</Filter>
    </ClCompile>'''

    # Self-closing tag를 찾아서 Filter 추가
    content = re.sub(
        r'    <ClCompile Include="(Generated\\[^"]+\.generated\.cpp)" />',
        add_filter,
        content
    )

    if content != original_content:
        filters_path.write_text(content, encoding='utf-8')
        print(f"✅ Updated: {filters_path.name}")
        return True
    else:
        print(f"⏭️  No changes needed: {filters_path.name}")
        return False


def main():
    if len(sys.argv) < 2:
        print("Usage: python update_filters.py <path-to-vcxproj.filters>")
        print("Example: python update_filters.py Mundi.vcxproj.filters")
        sys.exit(1)

    filters_path = Path(sys.argv[1])

    print("=" * 60)
    print("🔧 Updating .vcxproj.filters")
    print("=" * 60)
    print(f"📁 File: {filters_path}\n")

    update_filters_file(filters_path)

    print()
    print("=" * 60)
    print("✅ Update complete!")
    print("=" * 60)


if __name__ == '__main__':
    main()
