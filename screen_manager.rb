# screen_manager.rb is an example utility for managing a bank of USB
# connected LCD screens using libartista.rb
# Zoltan Olah (zol@me.com) released under the MIT license in 2007.
#!/usr/bin/env ruby

require 'logger'
require 'yaml'
require 'libartista'

class Screen
  attr_accessor :id, :image, :offline
  
  def initialize(id)
    @id = id
    @image = nil
    @offline = true
  end
  
  def show(image)
    @image = image
    @image.display_time = Time.now

    # TODO: this config ref should not be a global
    #image_path = File.dirname( __FILE__ ) + '/' + 
    image_path = @@config['config']['image_dir'] + '/' +
                 @image.file
                 
    if not FileTest.exists? image_path then
      @@log.error "Image file does not exist: #{image_path}"
      return false
    end
                     
    begin                   
      Artista.show(image_path, @id)
      @offline = false
    rescue Exception
      @offline = true
      @@log.error "Screen '#{@id}' is down!"
      
      return false # write fucked up
    end
    
    true # write worked
  end
  
  def == (other)
    return @id == other.id if other.instance_of? Screen 

    @id == other.to_s
  end
  
  def <=> (other)
    return @id <=> other.id if other.instance_of? Screen 

    @id <=> other.to_s    
  end
  
  def to_s
#    "[#{@id}:#{@image}]"
    @id
  end
end

class Image
  attr_accessor :id, :duration, :display_time, :file
  
  def initialize(id, duration, file)
    @id = id
    @duration = duration
    @file = file
    @display_time = nil
  end
  
  # returns -x if duration has not been met
  # returns  0 if duration exactly met
  # returns +x if duration has been met and exceeded
  # where x is s (float)
  def time_dif(now)
    (now.to_f - @display_time.to_f) - @duration
  end
  
  def to_s
    "#{@id || ''}"
  end
end

class Manager
  def initialize(screens)
    @screens = []    
    screens.each_key { |key| @screens << Screen.new(key) }
    
    @fifo = []
  end
  
  def add_image(image)
    @fifo << image
  end
  
  def add_images(images)
    images.each { |key, value|
      image = Image.new(key, value['duration'], value['file'])
      @fifo << image
    }
  end
  
  def fill
    #randomize the fifo if we should
    @fifo = @fifo.sort_by { rand } if @@config['config']['random_start']
    
    @screens.each { |screen|
      if screen.image.nil? then
        candidate = @fifo.shift
        
        if not candidate.nil? then
          screen.show(candidate)
        end
      end
    }
  end
  
  def switch
    switch_in_order
  end

  # processes screens from left to right looking for an image which
  # has been displayed for it's full duration. an image then gets pulled in
  # from the FIFO and the position is remembered so on next tick, processing
  # can be resumed from this position
  def switch_in_order
    @pos = 0 unless defined? @pos
    @pos = 0 if @pos == @screens.length - 1

    now = Time.now    
    
    @pos.upto(@screens.length - 1) { |i| 
      @pos = i      
      image = @screens[i].image
      
      if not image.nil? and image.time_dif(now) > 0 then
        # swap it
        @fifo << image
        @screens[i].show(@fifo.shift)
        break
      end      
    }
  end
  
  def check
    # check size of fifo
    if @fifo.empty? then
      #possibly insert a 'default' image here
      @@log.error 'Banging out, FIFO is empty :-/'
      exit
    end
  end
  
  def find_new_screens
    # see if any unknown screens have joined the system
    begin
      all_screens = Artista.get_ids()
    rescue Exception
      @@log.error "Failed to get the screen list."
      exit
    end
    
    # only relable screens with "" id's
    new_screens = all_screens.reject { |s| @screens.include?(s) || s != "" }
    
    if not new_screens.empty? then
      offline_screens = @screens.select { |s| s.offline }
      offline_screens.sort! #ensure we label the lowest id
      
      if not offline_screens.empty? then
        # relable the new screen
        old_id = new_screens.first
        new_id = offline_screens.first        
                
        @@log.info "Relabeling '#{old_id}' as '#{new_id}'"
        
        begin
          Artista.set_id(old_id, new_id)
        rescue Exception
          @@log.error "Failed to relable '#{old_id}' as '#{new_id}'"
        end        
      end
    end
  end
  
  def run(tick_time)
    loop do
      # if in debug mode, just print
      if @@config['config']['debug'] then
        print
      else
        check
        find_new_screens
        switch      
      end
      
      sleep tick_time
    end
  end
  
  def run_forked(tick_time)
    pid = fork
    if pid == nil then
      # signal handling
      Signal.trap("KILL") do
	#Artista.blank_all()
        exit
      end
      
      Process.setsid
      exit if fork #zap session leader

      #free file descriptors
      STDIN.reopen "/dev/null"
      STDOUT.reopen "/dev/null", "a"
      STDERR.reopen "/dev/null"
      
      # write pid file
      pid = Process.pid
      File.new("screen_manager_#{pid}.pid", "w").puts(pid) 

      run(tick_time)
    else
      exit
    end    
  end
  
  def print
    out = ''
    @screens.each { |s| out << "[#{s}:#{s.image.id}] "}
    @@log.debug out
  end
end

#### main ####
if ARGV.length != 1 then
  puts "usage: screen_manager.rb <config_file>"
  exit
end

@@config = YAML::load_file(ARGV.first)
log_device = @@config['config']['log_device']
log_device = STDOUT if log_device == 'STDOUT' #str convert
@@log = Logger.new( log_device )

# setup
m = Manager.new( @@config['screens'] )
m.add_images( @@config['images'] )
m.fill

# run
tick_time = @@config['config']['tick_time']
if (@@config['config']['forked']) then
  m.run_forked(tick_time)
else
  m.run(tick_time)
end
