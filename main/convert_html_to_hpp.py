"""
convert_html_to_hpp.py
功能：将HTML文件转换为HPP头文件，并添加自定义头尾注释
根据MOTOR_CONFIG环境变量动态调整通道数量
"""

import argparse
import os

def convert_html_to_hpp(input_file, output_file, header_str="", footer_str="", channel_count=8):
    try:
         # 读取并处理HTML内容
        processed_lines = []
        with open(input_file, 'r', encoding='utf-8') as f:
            for line in f:

                stripped = line.lstrip()
                stripped = stripped.rstrip()
                # 跳过空行
                if not stripped:
                    continue
                # 跳过以//开头的注释行
                if stripped.lstrip().startswith('//'):
                    continue
                #注意这样无法处理//在中间的情况,需要识别并排除''中的//,比如地址
                #还可以加上处理<->注释
                #或者简单一些,用htmlmin过一遍,然后去掉空行


                # 保留处理后的行
                processed_lines.append(stripped)
        
        # 将所有行合并为单个字符串（无换行符）
        compressed_html = ''.join(processed_lines)
        
        # 替换通道数量占位符
        compressed_html = compressed_html.replace('CHANNEL_COUNT_PLACEHOLDER', str(channel_count))
        
        # 拼接新内容
        new_content = f"{header_str}\n{compressed_html}\n{footer_str}"

        # 写入HPP文件
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(new_content)
            
        print(f"转换成功！生成文件: {output_file} (通道数: {channel_count})")

    except Exception as e:
        print(f"错误: {str(e)}")

if __name__ == "__main__":
    # 从环境变量获取通道数量，默认为8
    channel_count = int(os.environ.get('CHANNEL_COUNT', '8'))
    
    parser = argparse.ArgumentParser(description="HTML转HPP工具")
    parser.add_argument('-i', '--input', default='index.html', help='输入文件路径')
    parser.add_argument('-o', '--output', default='index.hpp', help='输出文件路径')
    parser.add_argument('--header', 
                        default=
                        '#pragma once\n'
                        '#include <string_view>\n\n' 
                        'constexpr std::string_view web = R"rawliteral(',
                          help='头部自定义字符串')
    
    parser.add_argument('--footer', default=
                        ')rawliteral";',
                          help='尾部自定义字符串')
    
    parser.add_argument('--channels', type=int, default=channel_count,
                        help=f'通道数量 (默认: {channel_count}, 从环境变量CHANNEL_COUNT读取)')
    
    args = parser.parse_args()
    
    convert_html_to_hpp(
        input_file=args.input,
        output_file=args.output,
        header_str=args.header,
        footer_str=args.footer,
        channel_count=args.channels
    )

