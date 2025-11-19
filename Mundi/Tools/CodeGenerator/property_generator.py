"""
프로퍼티 리플렉션 코드 생성 모듈
BEGIN_PROPERTIES / END_PROPERTIES 블록을 생성합니다.
"""

from jinja2 import Template
from header_parser import ClassInfo, Property
from typing import Dict, Optional


PROPERTY_TEMPLATE = """
BEGIN_PROPERTIES({{ class_name }})
{%- if mark_type == 'SPAWNABLE' %}
    MARK_AS_SPAWNABLE("{{ display_name }}", "{{ description }}")
{%- elif mark_type == 'COMPONENT' %}
    MARK_AS_COMPONENT("{{ display_name }}", "{{ description }}")
{%- endif %}
{%- for prop in properties %}
    {%- if prop.get_property_type_macro() == 'ADD_PROPERTY_RANGE' %}
    ADD_PROPERTY_RANGE({{ prop.type }}, {{ prop.name }}, "{{ prop.category }}", {{ prop.min_value }}f, {{ prop.max_value }}f, {{ 'true' if prop.editable else 'false' }}{% if prop.tooltip %}, "{{ prop.tooltip }}"{% endif %})
    {%- elif prop.get_property_type_macro() == 'ADD_PROPERTY_ARRAY' %}
    ADD_PROPERTY_ARRAY({{ prop.metadata.get('inner_type', 'EPropertyType::ObjectPtr') }}, {{ prop.name }}, "{{ prop.category }}", {{ 'true' if prop.editable else 'false' }}{% if prop.tooltip %}, "{{ prop.tooltip }}"{% endif %})
    {%- elif prop.get_property_type_macro() == 'ADD_PROPERTY_SCRIPT' %}
    ADD_PROPERTY_SCRIPT({{ prop.type }}, {{ prop.name }}, "{{ prop.category }}", "{{ prop.metadata.get('ScriptFileExtension', '') }}", {{ 'true' if prop.editable else 'false' }}{% if prop.tooltip %}, "{{ prop.tooltip }}"{% endif %})
    {%- else %}
    {{ prop.get_property_type_macro() }}({{ prop.type }}, {{ prop.name }}, "{{ prop.category }}", {{ 'true' if prop.editable else 'false' }}{% if prop.tooltip %}, "{{ prop.tooltip }}"{% endif %})
    {%- endif %}
{%- endfor %}
END_PROPERTIES()
"""


class PropertyGenerator:
    """프로퍼티 등록 코드 생성기"""

    def __init__(self):
        self.template = Template(PROPERTY_TEMPLATE)
        self.class_hierarchy: Dict[str, str] = {}  # class_name -> parent_name 매핑

    def set_class_hierarchy(self, classes):
        """
        클래스 계층 구조 설정
        Args:
            classes: List[ClassInfo] - 파싱된 모든 클래스 정보
        """
        self.class_hierarchy = {cls.name: cls.parent for cls in classes}

    def is_actor_descendant(self, class_name: str) -> bool:
        """
        주어진 클래스가 AActor의 자손인지 확인 (조상 클래스를 따라 올라가며 검사)
        Args:
            class_name: 확인할 클래스 이름
        Returns:
            AActor를 상속받았으면 True, 아니면 False
        """
        # AActor 자체는 False (직접 상속이 아니므로)
        if class_name == 'AActor':
            return False
        
        current = class_name
        visited = set()  # 순환 참조 방지
        
        while current and current not in visited:
            visited.add(current)
            
            # 부모 클래스 찾기
            parent = self.class_hierarchy.get(current)
            if not parent:
                # 부모를 찾을 수 없으면 종료
                break
            
            # 부모가 AActor이면 True
            if parent == 'AActor':
                return True
            
            # 부모로 이동
            current = parent
        
        return False

    def generate(self, class_info: ClassInfo) -> str:
        """ClassInfo로부터 BEGIN_PROPERTIES 블록 생성"""

        # mark_type 결정:
        # 1. AActor 자체는 MARK 없음
        # 2. AActor를 상속받은 클래스 (직접 또는 간접)는 MARK_AS_SPAWNABLE
        # 3. 나머지는 MARK_AS_COMPONENT
        mark_type = None
        if class_info.name == 'AActor':
            mark_type = None  # AActor는 MARK 없음
        elif class_info.parent == 'AActor' or self.is_actor_descendant(class_info.name):
            mark_type = 'SPAWNABLE'  # AActor 직접 또는 간접 상속
        else:
            mark_type = 'COMPONENT'  # 그 외 (컴포넌트 등)

        # DisplayName과 Description 결정
        display_name = class_info.display_name or class_info.name
        description = class_info.description or f"Auto-generated {class_info.name}"

        if not class_info.properties:
            # 프로퍼티가 없어도 기본 블록은 생성
            if mark_type == 'SPAWNABLE':
                mark_line = f'    MARK_AS_SPAWNABLE("{display_name}", "{description}")'
            elif mark_type == 'COMPONENT':
                mark_line = f'    MARK_AS_COMPONENT("{display_name}", "{description}")'
            else:
                mark_line = ''

            return f"""
BEGIN_PROPERTIES({class_info.name})
{mark_line}
END_PROPERTIES()
"""

        return self.template.render(
            class_name=class_info.name,
            mark_type=mark_type,
            display_name=class_info.display_name or class_info.name,
            description=class_info.description or f"Auto-generated {class_info.name}",
            properties=class_info.properties
        )
