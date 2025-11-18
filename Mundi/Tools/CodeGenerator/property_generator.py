"""
프로퍼티 리플렉션 코드 생성 모듈
BEGIN_PROPERTIES / END_PROPERTIES 블록을 생성합니다.
"""

from jinja2 import Template
from header_parser import ClassInfo, Property


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

    def generate(self, class_info: ClassInfo) -> str:
        """ClassInfo로부터 BEGIN_PROPERTIES 블록 생성"""

        # mark_type 결정:
        # 1. AActor 자체는 MARK 없음
        # 2. AActor를 상속받은 클래스는 MARK_AS_SPAWNABLE
        # 3. 나머지는 MARK_AS_COMPONENT
        mark_type = None
        if class_info.name == 'AActor':
            mark_type = None  # AActor는 MARK 없음
        elif class_info.parent == 'AActor':
            mark_type = 'SPAWNABLE'  # AActor 직접 상속
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
