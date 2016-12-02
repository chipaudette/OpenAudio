
%https://www.ncbi.nlm.nih.gov/pmc/articles/PMC4172289/


fs_Hz = 44100;      %sample rate
comp_ratio = 5;     %compression ratio.  "5" means 5:1
attack_sec = 0.005;  %doesn't end up being exact.  Error is dependent upon compression ratio
release_sec = 0.1;  %doesn't end up being exact.  Error is dependent upon compression ratio
%thresh_dBFS = -10;  %threshold for compression to start
thresh_dBFS = -10/(1-1/comp_ratio);

%generate test signal
t_sec = ([1:4*fs_Hz]-1)/fs_Hz;
freq_Hz = 2000;  %frequency of my test signal
wav = sqrt(2)*sin(2*pi*freq_Hz*t_sec);  %rms of 1.0 (aka 0 dB)

%make step change in amplitude
flag_isAttackTest = 1;  %do an attack or a release test?
if (flag_isAttackTest)
    levels_dBFS = [-25 0];%two amplitude levels
else
    levels_dBFS = [0 -25];  %two amplitude levels
end
t_bound_sec = 1;        %time of transition
I=find(t_sec < t_bound_sec);
wav(I) = sqrt(10.^(0.1*levels_dBFS(1)))*wav(I);
I=find(t_sec >= t_bound_sec);
wav(I) = sqrt(10.^(0.1*levels_dBFS(2)))*wav(I);

%% process the signal

%get signal power
wav_pow = wav.^2;

if (0)
    %extract the smoothed signal envlope
    env_time_const = 1e-3;
    b = 1-exp(-1/(env_time_const*fs_Hz));
    a = [1 -exp(-1/(env_time_const*fs_Hz))];
    new_wav_pow = filter(b,a,wav_pow);
else
    new_wav_pow = wav_pow;
end

%get power relative to threshold
thresh_pow_FS = 10.^(0.1*thresh_dBFS);
%wav_pow_rel_thresh = wav_pow ./ thresh_pow_FS;
wav_pow_rel_thresh = new_wav_pow ./ thresh_pow_FS;

%compute gain
switch 1
    case 1
        %use pow and log on wav.^2...plus sqrt(gain)
        gain_pow = ones(size(wav_pow_rel_thresh));
        I=find(wav_pow_rel_thresh > 1.0);
        gain_pow(I) = 10.^((1/comp_ratio - 1)*log10(wav_pow_rel_thresh(I)));
        gain = sqrt(gain_pow);
    case 2
        %use pow on abs(wav)...now sqrt(gain) needed!
        gain = ones(size(wav));
        abs_wav = abs(wav);
        thresh_FS = sqrt(thresh_pow_FS);
        I=find(abs_wav > thresh_FS);
        gain(I) = (thresh_FS.^(1-1/comp_ratio))./(abs_wav(I).^(1-1/comp_ratio));
        gain_pow = gain.^2;  %only needed for plotting
    case 3
        %use pow on wav.^2...plus sqrt(gain)
        gain_pow = ones(size(wav_pow));
        I=find(wav_pow > thresh_pow_FS);
        gain_pow(I) = (thresh_pow_FS.^(1-1/comp_ratio))./(wav_pow(I).^(1-1/comp_ratio));
        gain = sqrt(gain_pow);
end

%smooth gain via attack and release
attack_const = exp(-1/(attack_sec*fs_Hz));
release_const = exp(-1/(release_sec*fs_Hz));
if (1)
    %power space
    new_gain_pow = gain_pow;
    for I=2:length(gain)
        if (gain_pow(I) < new_gain_pow(I-1))
            %attack
            new_gain_pow(I) = new_gain_pow(I-1)*attack_const + gain_pow(I)*(1-attack_const);
        else
            %release
            new_gain_pow(I) = new_gain_pow(I-1)*release_const + gain_pow(I)*(1-release_const);
        end
    end
    %new_gain_pow = new_gain.^2;
    new_gain = sqrt(new_gain_pow);
else
    %linear space
    new_gain = gain;
    for I=2:length(gain)
        if (gain(I) < new_gain(I-1))
            %attack
            new_gain(I) = new_gain(I-1)*attack_const + gain(I)*(1-attack_const);
        else
            %release
            new_gain(I) = new_gain(I-1)*release_const + gain(I)*(1-release_const);
        end
    end
    new_gain_pow = new_gain.^2;
end

%rename
raw_gain_pow = gain_pow;
gain_pow = new_gain_pow;

new_wav = wav.*sqrt(gain_pow);

%% plots
figure;try;setFigureTallestWide;catch;end
ax=[];

subplot(4,2,1);
plot(t_sec,wav);
xlabel('Time (sec)');
ylabel('Raw Waveform');
ylim([-2 2]);
hold on; 
plot(xlim,sqrt(10.^(0.1*thresh_dBFS))*[1 1],'k--','linewidth',2);
plot(xlim,-sqrt(10.^(0.1*thresh_dBFS))*[1 1],'k--','linewidth',2);
hold off
ax(end+1)=gca;

subplot(4,2,2);
plot(t_sec,10*log10(wav_pow));
xlabel('Time (sec)');
ylabel({'Signal Power';'(dBFS)'});
ylim([-40 10]);
hold on; 
plot(xlim,thresh_dBFS*[1 1],'k--','linewidth',2);
hold off
ax(end+1)=gca;

subplot(4,2,3);
plot(t_sec,10*log10(new_wav_pow));
xlabel('Time (sec)');
ylabel({'Smoothed Signal';'Power (dBFS)'});
ylim([-40 10]);
hold on; 
plot(xlim,thresh_dBFS*[1 1],'k--','linewidth',2);
hold off
ax(end+1)=gca;


subplot(4,2,4);
plot(t_sec,10*log10(wav_pow_rel_thresh));
xlabel('Time (sec)');
ylabel({'Signal Power (dB)';'Rel Threshold'});
ylim([-20 20]);
hold on; 
plot(xlim,[0 0],'k--','linewidth',2);
hold off
ax(end+1)=gca;

subplot(4,2,5);
plot(t_sec,10*log10([raw_gain_pow(:) gain_pow(:)]));
xlabel('Time (sec)');
ylabel({'Target Gain';'(dB)'});
ylim([-12 0]);
hold on; 
plot(xlim,[0 0],'k--','linewidth',2);
hold off
ax(end+1)=gca;

subplot(4,2,6);
plot(t_sec,new_wav);
xlabel('Time (sec)');
ylabel('New Waveform');
ylim([-2 2]);
hold on; 
plot(xlim,sqrt(10.^(0.1*thresh_dBFS))*[1 1],'k--','linewidth',2);
plot(xlim,-sqrt(10.^(0.1*thresh_dBFS))*[1 1],'k--','linewidth',2);
hold off
ax(end+1)=gca;




% subplot(3,2,5);
% plot(t_sec,10*log10(new_gain_pow));
% xlabel('Time (sec)');
% ylabel('Smoothed Gain (dB)');
% ylim([-10 0]);
% hold on; 
% plot(xlim,[0 0],'k--','linewidth',2);
% hold off

linkaxes(ax,'x');
xlim([0.9 1.2]);

subplot(4,2,7)
dt_sec = t_sec-t_bound_sec;
end_gain = mean(gain_pow(end+[-fs_Hz/4:0]));
start_gain = mean(gain_pow(fs_Hz/4:fs_Hz/2));
if (flag_isAttackTest)
    scaled_gain = (gain_pow-end_gain)/(start_gain-end_gain);
else
    scaled_gain = (gain_pow-start_gain)/(end_gain-start_gain);
end
semilogx(dt_sec,scaled_gain);
xlabel('Time (sec) Since Transition');
ylabel({'Scaled Gain'});
%x(end+1)=gca;
xlim([1e-4 1]);
ylim([0 1]);

if (flag_isAttackTest)
    time_const_val = 1-0.63;
    hold on;plot(xlim,time_const_val*[1 1],'k--','linewidth',2);hold off
    I=find(scaled_gain > time_const_val);I=I(end)+1;

    hold on;plot(dt_sec(I)*[1 1],ylim,'k:','linewidth',2);hold off;
    yl=ylim;
    text(dt_sec(I),yl(2)-0.05*diff(yl),[num2str(dt_sec(I),4) ' sec'], ...
        'horizontalAlignment','center','verticalAlignment','top',...
        'backgroundcolor','white');
else
    time_const_val = 0.63;
    hold on;plot(xlim,time_const_val*[1 1],'k--','linewidth',2);hold off
    I=find(scaled_gain < time_const_val);I=I(end)+1;
 
    hold on;plot(dt_sec(I)*[1 1],ylim,'k:','linewidth',2);hold off;
    yl=ylim;
    text(dt_sec(I),yl(1)+0.05*diff(yl),[num2str(dt_sec(I),4) ' sec'], ...
        'horizontalAlignment','center','verticalAlignment','bottom',...
        'backgroundcolor','white');
end
   

% 
% subplot(4,2,8);
% dt_sec = t_sec-t_bound_sec;
% gain_rel_final_dB = 10*log10(gain_pow/gain_pow(end));
% semilogx(dt_sec,gain_rel_final_dB)
% xlabel('Time (sec) Since Transition');
% ylabel({'Gain (dB) Re:';'Final Gain (dB)'});
% xlim([1e-4 1]);
% if (flag_isAttackTest)
%     ylim([-1 10]);
%     time_const_val = 2;
%     hold on;plot(xlim,time_const_val*[1 1],'k--','linewidth',2);hold off
%     I=find(gain_rel_final_dB > time_const_val);I=I(end)+1;
% 
%     hold on;plot(dt_sec(I)*[1 1],ylim,'k:','linewidth',2);hold off;
%     yl=ylim;
%     text(dt_sec(I),yl(2)-0.05*diff(yl),[num2str(dt_sec(I),4) ' sec'], ...
%         'horizontalAlignment','center','verticalAlignment','top',...
%         'backgroundcolor','white');
% else
%     ylim([-10 1]);
%     time_const_val = -2;
%     hold on;plot(xlim,time_const_val*[1 1],'k--','linewidth',2);hold off
%     I=find(gain_rel_final_dB < time_const_val);I=I(end)+1;
%  
%     hold on;plot(dt_sec(I)*[1 1],ylim,'k:','linewidth',2);hold off;
%     yl=ylim;
%     text(dt_sec(I),yl(1)+0.05*diff(yl),[num2str(dt_sec(I),4) ' sec'], ...
%         'horizontalAlignment','center','verticalAlignment','bottom',...
%         'backgroundcolor','white');
% end
   



