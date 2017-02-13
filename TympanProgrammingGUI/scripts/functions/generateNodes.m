function [nodes] = generateNodes(origNode_fname,newNode_pname,outfname)

if nargin < 3
    %outfname = 'NewOutputs\new_nodes.txt'; 
    outfname = [];
    if nargin < 2
        %source location for header files for all of the new nodes
        newNode_pname = 'C:\Users\wea\Documents\Arduino\libraries\OpenAudio_ArduinoLibrary\';
        if nargin < 2
            %location of original node text from original index.html
            origNode_fname = 'ParsedInputs\nodes.txt';
        end
    end
end
    

addpath('functions\');

%% read existing node file and get the nodes
orig_nodes = parseNodeFile(origNode_fname);

% keep just a subsete of the nodes
% nodes_keep = {'AudioInputI2S','AudioInputUSB',...
%     'AudioOutputI2S','AudioOutputUSB',...
%     'AudioPlaySdWav',...
%     'AudioPlayQueue','AudioRecordQueue',...
%     'AudioSynthWaveformSine','AudioSynthWaveform','AudioSynthToneSweep',...
%     'AudioSynthNoiseWhite','AudioSynthNoisePink',...
%     'AudioAnalyzePeak','AudioAnalyzeRMS',...
%     'AudioControlSGTL5000'};

nodes_keep = {'AudioInputUSB',...
    'AudioOutputUSB',...
    'AudioPlaySdWav',...
    'AudioPlayQueue','AudioRecordQueue',...
    'AudioAnalyzePeak','AudioAnalyzeRMS'};


%keep just these
nodes=[];
for Ikeep=1:length(nodes_keep)
    for Iorig=1:length(orig_nodes)
        node = orig_nodes(Iorig);
        if strcmpi(node.type,nodes_keep{Ikeep})
            if isempty(nodes)
                nodes = node;
            else
                nodes(end+1) = node;
            end
        end
    end
end

%% add in new nodes

%To build text for the new nodes, use buildNewNodes.m.
%Then paste into XLSX to edit as desired.
%Then load the XLSX via the command below
if (0)
    [num,txt,raw]=xlsread('myNodes.xlsx');
    headings = raw(1,:);
    new_node_data = raw(2:end,:);
else
    %get info directly from underlying classes
    %source_pname = 'C:\Users\wea\Documents\Arduino\libraries\OpenAudio_ArduinoLibrary\';
    [headings, new_node_data]=buildNewNodes(newNode_pname);
end

%build new node data elements
template = nodes(1);
new_nodes=[];
for Inode = 1:size(new_node_data,1)
    node = template;
    for Iheading = 1:length(headings)
        node.(headings{Iheading}) = new_node_data{Inode,Iheading};
    end
    
    if isempty(nodes)
        nodes = node;
    else
        nodes(end+1) = node;
    end
end

%% write new nodes
if ~isempty(outfname)
    writeNodeText(nodes,outfname)
end

        

